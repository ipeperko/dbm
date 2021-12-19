#include "dbm/dbm.hpp"
#include "db_settings.h"
#ifdef DBM_MYSQL
#include <dbm/drivers/mysql/mysql_session.hpp>
#endif
#ifdef DBM_POSTGRES
#include <dbm/drivers/postgres/postgres_session.hpp>
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
    session.init(mysql_host, mysql_username, mysql_password, mysql_port, db_name);
}

std::unique_ptr<dbm::session> db_settings::get_mysql_session()
{
    auto db = std::make_unique<dbm::mysql_session>();
    db_settings::instance().init_mysql_session(*db, db_settings::instance().test_db_name);
    db->open();
    db->create_database(db_settings::instance().test_db_name, true);
    return db;
}
#endif

#ifdef DBM_POSTGRES
void db_settings::init_postgres_session(dbm::postgres_session& session, std::string const& db_name)
{
    session.init(postgres_host, postgres_username, postgres_password, postgres_port, db_name);
}

std::unique_ptr<dbm::session> db_settings::get_postgres_session()
{
    auto db = std::make_unique<dbm::postgres_session>();
    db_settings::instance().init_postgres_session(*db, db_settings::instance().test_db_name);
    return db;
}
#endif

#ifdef DBM_SQLITE3
std::unique_ptr<dbm::session> db_settings::get_sqlite_session()
{
    auto db = std::make_unique<dbm::sqlite_session>();
    db->set_db_name(db_settings::instance().test_db_file_name);
    db->open();
    return db;
}
#endif

void db_settings::initialize()
{
    int argc = framework::master_test_suite().argc;

    for (int i = 0; i < argc - 1; i++) {
        std::string arg = framework::master_test_suite().argv[i];

        if (arg == "--host") {
            mysql_host = framework::master_test_suite().argv[i + 1];
            postgres_host = framework::master_test_suite().argv[i + 1];
        }
        else if (arg == "--db-username") {
            mysql_username = framework::master_test_suite().argv[i + 1];
            postgres_username = framework::master_test_suite().argv[i + 1];
        }
        else if (arg == "--db-password") {
            mysql_password = framework::master_test_suite().argv[i + 1];
            postgres_password = framework::master_test_suite().argv[i + 1];
        }
        else if (arg == "--mysql-host") {
            mysql_host = framework::master_test_suite().argv[i + 1];
        }
        else if (arg == "--mysql-username") {
            mysql_username = framework::master_test_suite().argv[i + 1];
        }
        else if (arg == "--mysql-password") {
            mysql_password = framework::master_test_suite().argv[i + 1];
        }
        else if (arg == "--mysql-port") {
            mysql_port = std::stoi(framework::master_test_suite().argv[i + 1]);
        }
        else if (arg == "--postgres-host") {
            postgres_host = framework::master_test_suite().argv[i + 1];
        }
        else if (arg == "--postgres-username") {
            postgres_username = framework::master_test_suite().argv[i + 1];
        }
        else if (arg == "--postgres-password") {
            postgres_password = framework::master_test_suite().argv[i + 1];
        }
        else if (arg == "--postgres-port") {
            postgres_port = std::stoi(framework::master_test_suite().argv[i + 1]);
        }
        else if (i > 0) {
            throw std::runtime_error("Invalid argument '" + arg + "'");
        }

        if (i > 0) {
            i++;
        }
    }

#ifdef DBM_MYSQL
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
#ifdef DBM_POSTGRES
    if (postgres_host.empty()) {
        BOOST_FAIL("postgres host name not defined");
    }
    if (postgres_username.empty()) {
        BOOST_FAIL("postgres username not defined");
    }
    if (postgres_password.empty()) {
        BOOST_TEST_MESSAGE("postgres password not defined - using blank password");
    }
#endif
}

