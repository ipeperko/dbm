#include "db_settings.h"
#include "common.h"
#include "dbm/dbm.hpp"
#include <dbm/drivers/mysql/mysql_session.hpp>

using namespace boost::unit_test;

template<typename DBType>
void injected_statement_impl()
{
    auto conn = std::make_unique<DBType>();
    if constexpr (std::is_same_v<DBType, dbm::mysql_session>) {
        db_settings::instance().init_mysql_session(*conn);
    }
    else if constexpr (std::is_same_v<DBType, dbm::sqlite_session>) {
        conn->connect(db_settings::instance().test_db_file_name);
    }

    std::string id = "'a1'";
    std::string value = "test";

    dbm::model tbl_data("injected_data",
                        {
                            { dbm::key("id"), dbm::binding(id),
                              dbm::primary(true), dbm::auto_increment(true), dbm::not_null(true), dbm::valquotes(false),
                              dbm::custom_data_type("VARCHAR(128)") },
                            { dbm::key("value"), dbm::binding(value) },
                        });
    tbl_data.drop_table(*conn);
    tbl_data.create_table(*conn);

    dbm::model tbl_control("injected_control",
                           {
                               { dbm::key("id"), dbm::local<int>(), dbm::primary(true), dbm::auto_increment(true), dbm::not_null(true)},
                               { dbm::key("value"), dbm::binding(value) },
                           });
    tbl_control.drop_table(*conn);
    tbl_control.create_table(*conn);

    value = "test";
    tbl_data >> *conn;
    tbl_control >> *conn;

    // check initial record
    auto rows_data = conn->select("SELECT * FROM injected_data");
    BOOST_TEST(rows_data.size() == 1);
    auto rows_control = conn->select("SELECT * FROM injected_control");
    BOOST_TEST(rows_control.size() == 1);

    // try inject via model
    id += "; DELETE FROM injected_control";
    BOOST_CHECK_THROW((*conn >> tbl_data), std::exception);
    // still has the initial record?
    rows_control = conn->select("SELECT * FROM injected_control");
    BOOST_TEST(rows_control.size() == 1);

    // try inject via 'query'
    BOOST_CHECK_THROW(conn->query("SELECT * FROM injected_data WHERE id=" + id), std::exception);
    // still have the initial record?
    rows_control = conn->select("SELECT * FROM injected_control");
    BOOST_TEST(rows_control.size() == 1);

    // try inject via 'select'
    BOOST_CHECK_THROW(conn->select("SELECT * FROM injected_data WHERE id=" + id), std::exception);
    // still have the initial record?
    rows_control = conn->select("SELECT * FROM injected_control");
    BOOST_TEST(rows_control.size() == 1);
}

BOOST_AUTO_TEST_SUITE(TstInjectedStatement)

BOOST_AUTO_TEST_CASE(injected_statement)
{
#ifdef DBM_MYSQL
    BOOST_TEST_MESSAGE("MySQL test\n");
    injected_statement_impl<dbm::mysql_session>();
#endif
#ifdef DBM_SQLITE3
    // TODO: sqlite fails
    //BOOST_TEST_MESSAGE("SQLite test\n");
    //injected_statement_impl<dbm::sqlite_session>();
#endif
}

BOOST_AUTO_TEST_SUITE_END()
