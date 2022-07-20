#include "dbm/dbm.hpp"
#include "db_settings.h"
#include "common.h"

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

template<typename DBType>
class Test
{
public:

    Test()
        : session_(db_settings::instance().get_session<DBType>())
    {
    }

    void insert_impl();
    void upsert_impl();
    void insert2_impl();
    void select_impl();
    void close_impl();

    void run();

//    auto& session() { return *session_; }

private:
    std::unique_ptr<DBType> session_;
};

template<typename DBType>
void Test<DBType>::insert_impl()
{
    Person person;
    auto m = get_person_model(person);

    m.drop_table(*session_);
    m.create_table(*session_);

    dbm::prepared_stmt stmt (original_insert_statement,
                            &m.at(1).get(),
                            &m.at(2).get(),
                            &m.at(3).get());
    BOOST_TEST(session_->prepared_statement_handles().size() == 0);

    // 1
    person.name = "Tarzan";
    person.age = 20;
    person.weight = 75.5;
    stmt >> *session_;
    BOOST_TEST(session_->prepared_statement_handles().size() == 1);

    // 2
    person.name = "Jane";
    person.age = 18;
    person.weight = 62;
    stmt >> *session_;
    BOOST_TEST(session_->prepared_statement_handles().size() == 1);

    // 3
    person.name = "Chita";
    person.age = 8;
    person.weight = 51;
    stmt.param(2)->set_null(true);
    stmt >> *session_;
    BOOST_TEST(session_->prepared_statement_handles().size() == 1);

    // another instance with identical statement should reuse existing handle
    {
        // 4
        dbm::prepared_stmt stmt2 (original_insert_statement,
                                 dbm::local<std::string>("Alien"),
                                 dbm::local(99),
                                 dbm::local(100.0));
        stmt2 >> *session_;
        BOOST_TEST(session_->prepared_statement_handles().size() == 1);
    }

    BOOST_TEST(session_->prepared_statement_handles().size() == 1);

    // 5
    // write null values
    m.at("name").set_value(nullptr);
    m.at("age").set_value(nullptr);
    m.at("weight").set_value(nullptr);
    stmt >> *session_;

    // check entry 1
    person.reset(1);
    *session_ >> m;
    BOOST_TEST(person.name == "Tarzan");
    BOOST_TEST(person.age == 20);
    BOOST_TEST(person.weight == 75.5);

    // check entry 2
    person.reset(2);
    *session_ >> m;
    BOOST_TEST(person.name == "Jane");
    BOOST_TEST(person.age == 18);
    BOOST_TEST(person.weight == 62);

    // check entry 3
    person.reset(3);
    *session_ >> m;
    BOOST_TEST(person.name == "Chita");
    BOOST_TEST(person.age == 8);
    BOOST_TEST(m.at("weight").is_null());

    // check entry 4
    person.reset(4);
    *session_ >> m;
    BOOST_TEST(person.name == "Alien");
    BOOST_TEST(person.age == 99);
    BOOST_TEST(person.weight == 100);

    // check entry 5 (all null)
    person.reset(5);
    *session_ >> m;
    BOOST_TEST(m.at("name").is_null());
    BOOST_TEST(m.at("age").is_null());
    BOOST_TEST(m.at("weight").is_null());
}

template<typename DBType>
void Test<DBType>::upsert_impl()
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

    session_->query("DROP FUNCTION IF EXISTS upsert_test_prepared_stmt");
    session_->query(upsert_statement.data());

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
    stmt >> *session_;

    person.reset(99);
    *session_ >> m;
    BOOST_TEST(m.at("name").value<std::string>() == "ivo");

    m.at("name").set_value(nullptr);
    stmt >> *session_;
    reset_model();
    *session_ >> m;
    BOOST_TEST(m.at("name").is_null());

    m.at("name").set_value("ivo no age");
    m.at("age").set_value(nullptr);
    stmt >> *session_;
    reset_model();
    *session_ >> m;
    BOOST_TEST(m.at("name").value<std::string>() == "ivo no age");
    BOOST_TEST(m.at("age").is_null());
    BOOST_TEST(!m.at("weight").is_null());

    m.at("name").set_value("ivo no weight");
    m.at("age").set_value(40);
    m.at("weight").set_value(nullptr);
    stmt >> *session_;
    reset_model();
    *session_ >> m;
    BOOST_TEST(m.at("name").value<std::string>() == "ivo no weight");
    BOOST_TEST(!m.at("age").is_null());
    BOOST_TEST(m.at("weight").is_null());
}

template<typename DBType>
void Test<DBType>::insert2_impl()
{
    // 7
    dbm::prepared_stmt stmt (original_insert_statement,
                            dbm::local<std::string>("Snoopy"),
                            dbm::local(11),
                            dbm::local(19));
    stmt >> *session_;
}

template<typename DBType>
void Test<DBType>::select_impl()
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

    auto res = session_->select(stmt);

    BOOST_TEST(res.size() == 7);
    BOOST_TEST(!res[0][1]->is_null());
    BOOST_TEST(res[0][1]->template get<std::string>() == "Tarzan");
    BOOST_TEST(res[0][2]->template get<int>() == 20);
    BOOST_TEST(res[0][3]->template get<double>() == 75.5);
    BOOST_TEST(res[4][1]->is_null());
    BOOST_TEST(res[4][2]->is_null());
    BOOST_TEST(res[4][3]->is_null());
    BOOST_TEST(res[5][1]->template get<std::string>() == "ivo no weight");
}

template<typename DBType>
void Test<DBType>::close_impl()
{
    dbm::prepared_stmt stmt (original_insert_statement,
                            dbm::local<std::string>(""),
                            dbm::local<int>(),
                            dbm::local<double>());

    // 8
    // execute statement just to make sure this one works
    session_->query(stmt);

    int nrows = session_->select(dbm::statement() << "SELECT count(*) AS count FROM test_prepared_stmt").at(0).at("count").template get<int>();
    BOOST_TEST(nrows == 8);

    // should close all handles on close
    session_->close();
    BOOST_TEST(session_->prepared_statement_handles().size() == 0);

    // should throw since the handle becomes invalid after close
    BOOST_REQUIRE_THROW(session_->query(stmt), std::exception);
}

template<typename DBType>
void Test<DBType>::run()
{
    BOOST_TEST_CHECKPOINT("insert_impl");
    insert_impl();

    BOOST_TEST_CHECKPOINT("upsert_impl");
    if constexpr (std::is_same_v<DBType, dbm::mysql_session>) {
        upsert_impl();
    }
    else if constexpr (std::is_same_v<DBType, dbm::sqlite_session>) {
        // 6
        // SQLite has no stored procedures support.
        // We will just insert data required for the next test step.
        dbm::prepared_stmt stmt ("INSERT INTO test_prepared_stmt (id, name, age, weight) VALUES(?, ?, ?, ?)",
                                dbm::local<int>(99),
                                dbm::local<std::string>("ivo no weight"),
                                dbm::local<int>(40),
                                dbm::local<double>());
        stmt >> *session_;
    }

    BOOST_TEST(session_->prepared_statement_handles().size() == 2);

    BOOST_TEST_CHECKPOINT("insert2_impl");
    insert2_impl();

    BOOST_TEST_CHECKPOINT("select_impl");
    select_impl();

    BOOST_TEST_CHECKPOINT("close_impl");
    close_impl();
}

} // namespace


BOOST_AUTO_TEST_SUITE(TstPreparedStmt)

BOOST_AUTO_TEST_CASE(prepared_stmt_basic)
{
//    Test test;

#ifdef DBM_MYSQL
    BOOST_TEST_CHECKPOINT("prepared statement basic MySql");
    Test<dbm::mysql_session>().run();

#endif

#ifdef DBM_SQLITE3
    BOOST_TEST_CHECKPOINT("prepared statement basic SQLite");
    Test<dbm::sqlite_session>().run();
#endif
}

BOOST_AUTO_TEST_SUITE_END()
