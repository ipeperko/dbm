#include "dbm/dbm.hpp"
#include "db_settings.h"
#include "common.h"
#ifdef DBM_MYSQL
#include <dbm/drivers/mysql/mysql_session.hpp>
#endif
#ifdef DBM_SQLITE3
#include <dbm/drivers/sqlite/sqlite_session.hpp>
#endif
#include <atomic>

using namespace boost::unit_test;

namespace{

constexpr const char* original_insert_statement = "INSERT INTO test_prepared_stmt (name, age, weight) VALUES(?, ?, ?)";

struct Person
{
    int id {1};
    std::string name;
    int age {0};
    double weight {0};

    void reset(std::optional<int> new_id = std::nullopt)
    {
        *this = {};
        if (new_id)
            id = new_id.value();
    }
};

auto get_person_model(Person& person)
{
    return dbm::model {
        "test_prepared_stmt",
        {
            { dbm::key("id"), dbm::binding(person.id), dbm::primary(true), dbm::not_null(true), dbm::auto_increment(true) },
            { dbm::key("name"), dbm::binding(person.name) },
            { dbm::key("age"), dbm::binding(person.age) },
            { dbm::key("weight"), dbm::binding(person.weight) }
        }
    };
}

class Test
{
public:

    Test()
    {
#ifdef DBM_MYSQL
        {
            auto db = std::make_unique<dbm::mysql_session>();
            db_settings::instance().init_mysql_session(*db, db_settings::instance().test_db_name);
            db->open();
            db->create_database(db_settings::instance().test_db_name, true);
            mysql_session_ = std::move(db);
        }
#endif
#ifdef DBM_SQLITE3
        {
            auto db = std::make_unique<dbm::sqlite_session>();
            db->set_db_name(db_settings::instance().test_db_file_name);
            db->open();
            sqlite_session_ = std::move(db);
        }
#endif
    }

    void insert_impl(dbm::session& session);
    void upsert_impl(dbm::session& session);
    void insert2_impl(dbm::session& session);
    void select_impl(dbm::session& session);
    void close_impl(dbm::session& session);

    template<typename SessionType>
    void run(dbm::session& session);

    dbm::session& mysql_session() { return *mysql_session_; }
    dbm::session& sqlite_session() { return *sqlite_session_; }

private:
    std::unique_ptr<dbm::session> mysql_session_;
    std::unique_ptr<dbm::session> sqlite_session_;
};

void Test::insert_impl(dbm::session& session)
{
    Person person;
    auto m = get_person_model(person);

    m.drop_table(session);
    m.create_table(session);

    dbm::prepared_stmt stmt (original_insert_statement,
                            &m.at(1).get(),
                            &m.at(2).get(),
                            &m.at(3).get());
    BOOST_TEST(session.prepared_statement_handles().size() == 0);

    // 1
    person.name = "Tarzan";
    person.age = 20;
    person.weight = 75.5;
    stmt >> session;
    BOOST_TEST(session.prepared_statement_handles().size() == 1);

    // 2
    person.name = "Jane";
    person.age = 18;
    person.weight = 62;
    stmt >> session;
    BOOST_TEST(session.prepared_statement_handles().size() == 1);

    // 3
    person.name = "Chita";
    person.age = 8;
    person.weight = 51;
    stmt.param(2)->set_null(true);
    stmt >> session;
    BOOST_TEST(session.prepared_statement_handles().size() == 1);

    // another instance with identical statement should reuse existing handle
    {
        // 4
        dbm::prepared_stmt stmt2 (original_insert_statement,
                                 dbm::local<std::string>("Alien"),
                                 dbm::local(99),
                                 dbm::local(100.0));
        stmt2 >> session;
        BOOST_TEST(session.prepared_statement_handles().size() == 1);
    }

    BOOST_TEST(session.prepared_statement_handles().size() == 1);

    // 5
    // write null values
    m.at("name").set_value(nullptr);
    m.at("age").set_value(nullptr);
    m.at("weight").set_value(nullptr);
    stmt >> session;

    // check entry 1
    person.reset(1);
    session >> m;
    BOOST_TEST(person.name == "Tarzan");
    BOOST_TEST(person.age == 20);
    BOOST_TEST(person.weight == 75.5);

    // check entry 2
    person.reset(2);
    session >> m;
    BOOST_TEST(person.name == "Jane");
    BOOST_TEST(person.age == 18);
    BOOST_TEST(person.weight == 62);

    // check entry 3
    person.reset(3);
    session >> m;
    BOOST_TEST(person.name == "Chita");
    BOOST_TEST(person.age == 8);
    BOOST_TEST(m.at("weight").is_null());

    // check entry 4
    person.reset(4);
    session >> m;
    BOOST_TEST(person.name == "Alien");
    BOOST_TEST(person.age == 99);
    BOOST_TEST(person.weight == 100);

    // check entry 5 (all null)
    person.reset(5);
    session >> m;
    BOOST_TEST(m.at("name").is_null());
    BOOST_TEST(m.at("age").is_null());
    BOOST_TEST(m.at("weight").is_null());
}

void Test::upsert_impl(dbm::session& session)
{
    Person person;
    auto m = get_person_model(person);

    std::string_view upsert_statement =
        R"(CREATE FUNCTION upsert_test_prepared_stmt(pid INT, pname Varchar(50), page int, pweight DOUBLE)
RETURNS INT DETERMINISTIC
BEGIN
	INSERT INTO test_prepared_stmt (id, name, age, weight)
	VALUES (pid, pname, page, pweight)
	ON DUPLICATE KEY UPDATE name=pname, age=page, weight=pweight;
	RETURN 1;
END)";

    auto reset_model = [&]() {
        m.at("name").set_value("xxx");
        m.at("age").set_value(-1);
        m.at("weight").set_value(-1);
    };

    session.query("DROP FUNCTION IF EXISTS upsert_test_prepared_stmt");
    session.query(upsert_statement.data());

    dbm::prepared_stmt stmt ("SELECT upsert_test_prepared_stmt(?, ?, ?, ?)",
                            &m.at(0).get(),
                            &m.at(1).get(),
                            &m.at(2).get(),
                            &m.at(3).get());

    person.id = 99;
    person.name = "ivo";
    person.age = 40;
    person.weight = 80;

    // 6
    stmt >> session;

    person.reset(99);
    session >> m;
    BOOST_TEST(m.at("name").value<std::string>() == "ivo");

    m.at("name").set_value(nullptr);
    stmt >> session;
    reset_model();
    session >> m;
    BOOST_TEST(m.at("name").is_null());

    m.at("name").set_value("ivo no age");
    m.at("age").set_value(nullptr);
    stmt >> session;
    reset_model();
    session >> m;
    BOOST_TEST(m.at("name").value<std::string>() == "ivo no age");
    BOOST_TEST(m.at("age").is_null());
    BOOST_TEST(!m.at("weight").is_null());

    m.at("name").set_value("ivo no weight");
    m.at("age").set_value(40);
    m.at("weight").set_value(nullptr);
    stmt >> session;
    reset_model();
    session >> m;
    BOOST_TEST(m.at("name").value<std::string>() == "ivo no weight");
    BOOST_TEST(!m.at("age").is_null());
    BOOST_TEST(m.at("weight").is_null());
}

void Test::insert2_impl(dbm::session& session)
{
    // 7
    dbm::prepared_stmt stmt (original_insert_statement,
                            dbm::local<std::string>("Snoopy"),
                            dbm::local(11),
                            dbm::local(19));
    stmt >> session;
}

void Test::select_impl(dbm::session& session)
{
    Person person;
    auto m = get_person_model(person);

    dbm::prepared_stmt stmt ("SELECT * FROM test_prepared_stmt",
                            &*m.at(0),
                            &*m.at(1),
                            &*m.at(2),
                            &*m.at(3)
    );

    person.id = 1;

    auto res = session.select(stmt);

    BOOST_TEST(res.size() == 7);
    BOOST_TEST(!res[0][1]->is_null());
    BOOST_TEST(res[0][1]->get<std::string>() == "Tarzan");
    BOOST_TEST(res[0][2]->get<int>() == 20);
    BOOST_TEST(res[0][3]->get<double>() == 75.5);
    BOOST_TEST(res[4][1]->is_null());
    BOOST_TEST(res[4][2]->is_null());
    BOOST_TEST(res[4][3]->is_null());
    BOOST_TEST(res[5][1]->get<std::string>() == "ivo no weight");
}

void Test::close_impl(dbm::session& session)
{
    dbm::prepared_stmt stmt (original_insert_statement,
                            dbm::local<std::string>(""),
                            dbm::local<int>(),
                            dbm::local<double>());

    // 8
    // execute statement just to make sure this one works
    session.query(stmt);

    int nrows = session.select(dbm::statement() << "SELECT count(*) AS count FROM test_prepared_stmt").at(0).at("count").get<int>();
    BOOST_TEST(nrows == 8);

    // should close all handles on close
    session.close();
    BOOST_TEST(session.prepared_statement_handles().size() == 0);

    // should throw since the handle becomes invalid after close
    BOOST_REQUIRE_THROW(session.query(stmt), std::exception);
}

template<typename SessionType>
void Test::run(dbm::session& session)
{
    BOOST_TEST_CHECKPOINT("insert_impl");
    insert_impl(session);

    BOOST_TEST_CHECKPOINT("upsert_impl");
    if constexpr (std::is_same_v<SessionType, dbm::mysql_session>) {
        upsert_impl(session);
    }
    else if constexpr (std::is_same_v<SessionType, dbm::sqlite_session>) {
        // 6
        // SQLite has no stored procedures support.
        // We will just insert data required for the next test.
        dbm::prepared_stmt stmt ("INSERT INTO test_prepared_stmt (id, name, age, weight) VALUES(?, ?, ?, ?)",
                                dbm::local<int>(99),
                                dbm::local<std::string>("ivo no weight"),
                                dbm::local<int>(40),
                                dbm::local<double>());
        stmt >> session;
    }

    BOOST_TEST(session.prepared_statement_handles().size() == 2);

    BOOST_TEST_CHECKPOINT("insert2_impl");
    insert2_impl(session);

    BOOST_TEST_CHECKPOINT("select_impl");
    select_impl(session);

    BOOST_TEST_CHECKPOINT("close_impl");
    close_impl(session);
}

} // namespace


BOOST_AUTO_TEST_SUITE(TstPreparedStmt)

BOOST_AUTO_TEST_CASE(prepared_stmt_basic)
{
    Test test;

#ifdef DBM_MYSQL
    BOOST_TEST_CHECKPOINT(__FUNCTION__ + std::string(" ... MySql"));
    test.run<dbm::mysql_session>(test.mysql_session());
#endif

#ifdef DBM_SQLITE3
    BOOST_TEST_CHECKPOINT(__FUNCTION__ + std::string(" ... SQLite"));
    test.run<dbm::sqlite_session>(test.sqlite_session());
#endif
}

BOOST_AUTO_TEST_SUITE_END()
