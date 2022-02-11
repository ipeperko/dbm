#include "db_settings.h"
#ifdef DBM_MYSQL
#include <dbm/drivers/mysql/mysql_session.hpp>
#endif
#ifdef DBM_SQLITE3
#include <dbm/drivers/sqlite/sqlite_session.hpp>
#endif

#include <boost/test/unit_test.hpp>

using namespace boost::unit_test;

db_settings::db_settings()
{
    initialize();
}

db_settings& db_settings::instance()
{
    static db_settings inst;
    return inst;
}

#ifdef DBM_MYSQL
void db_settings::init_mysql_session(dbm::mysql_session& session, std::string const& db_name)
{
    session.connect(mysql_host, mysql_username, mysql_password, db_name, mysql_port, std::nullopt, 0);
}

void db_settings::init_mysql_session(dbm::session& session, std::string const& db_name)
{
    init_mysql_session(dynamic_cast<dbm::mysql_session&>(session), db_name);
}

std::unique_ptr<dbm::session> db_settings::get_mysql_session()
{
    auto db = std::make_unique<dbm::mysql_session>();
    db_settings::instance().init_mysql_session(*db, test_db_name);
    db->create_database(test_db_name, true);
    return db;
}
#endif

#ifdef DBM_SQLITE3
void db_settings::init_sqlite_session(dbm::sqlite_session& session)
{
    session.connect(test_db_file_name);
}

void db_settings::init_sqlite_session(dbm::session& session)
{
    init_sqlite_session(dynamic_cast<dbm::sqlite_session&>(session));
}

std::unique_ptr<dbm::session> db_settings::get_sqlite_session()
{
    auto db = std::make_unique<dbm::sqlite_session>();
    db->connect(test_db_file_name);
    return db;
}
#endif

void db_settings::initialize()
{
#ifdef DBM_MYSQL
    int argc = framework::master_test_suite().argc;

    for (int i = 0; i < argc - 1; i++) {
        std::string arg = framework::master_test_suite().argv[i];

        if (arg == "--mysql-host") {
            mysql_host = framework::master_test_suite().argv[i + 1];
        }
        else if (arg == "--mysql-username" || arg == "--db-username") {
            mysql_username = framework::master_test_suite().argv[i + 1];
        }
        else if (arg == "--mysql-password" || arg == "--db-password") {
            mysql_password = framework::master_test_suite().argv[i + 1];
        }
    }

    if (mysql_host.empty()) {
        BOOST_FAIL("mysql host name not defined");
    }
    if (mysql_username.empty()) {
        BOOST_FAIL("mysql username not defined");
    }
    if (mysql_password.empty()) {
        BOOST_TEST_MESSAGE("mysql password not defined - using blank password");
    }
#endif
}

#ifdef DBM_MYSQL
std::shared_ptr<dbm::mysql_session> MakeMySqlSession::operator()()
{
    auto conn = std::make_shared<dbm::mysql_session>();
    db_settings::instance().init_mysql_session(*conn);
    return conn;
}
#endif

#ifdef DBM_SQLITE3
std::shared_ptr<dbm::sqlite_session> MakeSQLiteSession::operator()()
{
    auto conn = std::make_shared<dbm::sqlite_session>();
    db_settings::instance().init_sqlite_session(*conn);
    return conn;
}
#endif