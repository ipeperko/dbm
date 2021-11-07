#include "dbm/dbm.hpp"
#include "db_settings.h"
#include "common.h"
#ifdef DBM_MYSQL
#include <dbm/drivers/mysql/mysql_session.hpp>
#endif
#include <atomic>

using namespace boost::unit_test;

namespace{

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

std::unique_ptr<dbm::session> get_test_session()
{
    auto session = std::make_unique<dbm::mysql_session>();
    db_settings::instance().init_mysql_session(*session, db_settings::instance().test_db_name);
    session->open();
    session->create_database(db_settings::instance().test_db_name, true);
    return session;
}

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

} // namespace

BOOST_AUTO_TEST_SUITE(TstPreparedStmt)

BOOST_AUTO_TEST_CASE(prepared_stmt_insert)
{
    auto session = get_test_session();
    Person person;
    auto m = get_person_model(person);

    m.drop_table(*session);
    m.create_table(*session);

    dbm::prepared_stmt stmt ("INSERT INTO test_prepared_stmt (name, age, weight) VALUES(?, ?, ?)",
                            &m.at(1).get(),
                            &m.at(2).get(),
                            &m.at(3).get());
    BOOST_TEST(session->prepared_statement_handles().size() == 0);

    // 1
    person.name = "Tarzan";
    person.age = 20;
    person.weight = 75.5;
    stmt >> *session;
    BOOST_TEST(session->prepared_statement_handles().size() == 1);

    // 2
    person.name = "Jane";
    person.age = 18;
    person.weight = 62;
    stmt >> *session;
    BOOST_TEST(session->prepared_statement_handles().size() == 1);

    // 3
    person.name = "Chita";
    person.age = 8;
    person.weight = 51;
    stmt.param(2)->set_null(true);
    stmt >> *session;
    BOOST_TEST(session->prepared_statement_handles().size() == 1);

    // another instance with identical statement should reuse existing handle
    {
        // 4
        dbm::prepared_stmt stmt2 ("INSERT INTO test_prepared_stmt (name, age, weight) VALUES(?, ?, ?)",
                                 dbm::local<std::string>("Alien"),
                                 dbm::local(99),
                                 dbm::local(100.0));
        stmt2 >> *session;
        BOOST_TEST(session->prepared_statement_handles().size() == 1);
    }

    BOOST_TEST(session->prepared_statement_handles().size() == 1);

    // write null values
    m.at("name").set_value(nullptr);
    m.at("age").set_value(nullptr);
    m.at("weight").set_value(nullptr);
    stmt >> *session;

    // check entry 1
    person.reset(1);
    *session >> m;
    BOOST_TEST(person.name == "Tarzan");
    BOOST_TEST(person.age == 20);
    BOOST_TEST(person.weight == 75.5);

    // check entry 2
    person.reset(2);
    *session >> m;
    BOOST_TEST(person.name == "Jane");
    BOOST_TEST(person.age == 18);
    BOOST_TEST(person.weight == 62);

    // check entry 3
    person.reset(3);
    *session >> m;
    BOOST_TEST(person.name == "Chita");
    BOOST_TEST(person.age == 8);
    BOOST_TEST(m.at("weight").is_null());

    // check entry 4
    person.reset(4);
    *session >> m;
    BOOST_TEST(person.name == "Alien");
    BOOST_TEST(person.age == 99);
    BOOST_TEST(person.weight == 100);

    // check entry 5 (all null)
    person.reset(5);
    *session >> m;
    BOOST_TEST(m.at("name").is_null());
    BOOST_TEST(m.at("age").is_null());
    BOOST_TEST(m.at("weight").is_null());

    // should close all handles on close
    session->close();
    BOOST_TEST(session->prepared_statement_handles().size() == 0);

    // should throw since the handle becomes invalid after close
    BOOST_REQUIRE_THROW(session->query(stmt), std::exception);
}

BOOST_AUTO_TEST_CASE(prepared_stmt_upsert)
{
    auto session = get_test_session();
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

    session->query("DROP FUNCTION IF EXISTS upsert_test_prepared_stmt");
    session->query(upsert_statement.data());

    dbm::prepared_stmt stmt ("SELECT upsert_test_prepared_stmt(?, ?, ?, ?)",
                            &m.at(0).get(),
                            &m.at(1).get(),
                            &m.at(2).get(),
                            &m.at(3).get());

    person.id = 99;
    person.name = "ivo";
    person.age = 40;
    person.weight = 80;

    stmt >> *session;

    person.reset(99);
    *session >> m;
    BOOST_TEST(m.at("name").value<std::string>() == "ivo");

    m.at("name").set_value(nullptr);
    stmt >> *session;
    reset_model();
    *session >> m;
    BOOST_TEST(m.at("name").is_null());

    m.at("name").set_value("ivo no age");
    m.at("age").set_value(nullptr);
    stmt >> *session;
    reset_model();
    *session >> m;
    BOOST_TEST(m.at("name").value<std::string>() == "ivo no age");
    BOOST_TEST(m.at("age").is_null());
    BOOST_TEST(!m.at("weight").is_null());

    m.at("name").set_value("ivo no weight");
    m.at("age").set_value(40);
    m.at("weight").set_value(nullptr);
    stmt >> *session;
    reset_model();
    *session >> m;
    BOOST_TEST(m.at("name").value<std::string>() == "ivo no weight");
    BOOST_TEST(!m.at("age").is_null());
    BOOST_TEST(m.at("weight").is_null());
}

BOOST_AUTO_TEST_CASE(prepared_stmt_select)
{
    auto session = get_test_session();
    Person person;
    auto m = get_person_model(person);

    dbm::prepared_stmt stmt ("SELECT * FROM test_prepared_stmt",
                            &*m.at(0),
                            &*m.at(1),
                            &*m.at(2),
                            &*m.at(3)
                            );

    person.id = 1;

    auto res = session->select(stmt);

    BOOST_TEST(res.size() == 6);
    BOOST_TEST(!res[0][1]->is_null());
    BOOST_TEST(res[0][1]->get<std::string>() == "Tarzan");
    BOOST_TEST(res[0][2]->get<int>() == 20);
    BOOST_TEST(res[0][3]->get<double>() == 75.5);
    BOOST_TEST(res[4][1]->is_null());
    BOOST_TEST(res[4][2]->is_null());
    BOOST_TEST(res[4][3]->is_null());
    BOOST_TEST(res[5][1]->get<std::string>() == "ivo no weight");
}

BOOST_AUTO_TEST_SUITE_END()