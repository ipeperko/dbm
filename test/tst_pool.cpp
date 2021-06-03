#ifdef DBM_MYSQL

#include "dbm/dbm.hpp"
#include "db_settings.h"
#ifdef DBM_MYSQL
#include <dbm/drivers/mysql/mysql_session.hpp>
#endif

//#define BOOST_TEST_MODULE dbm_pool
#include <boost/test/unit_test.hpp>
#include <dbm/impl/session.ipp>
using namespace boost::unit_test;

namespace{
dbm::pool pool;

constexpr const char* db_name = "dbm_pool_unit_test";
}

BOOST_AUTO_TEST_SUITE(TstBasicTypes)

BOOST_AUTO_TEST_CASE(pool_init)
{
    auto& db_sett = db_settings::instance();
//    dbm::mysql_session setup_session;
//    setup_session.init(db_sett.mysql_host, db_sett.mysql_password, db_sett.mysql_password, db_sett.mysql_port, "");
//    setup_session.open();
//    setup_session.session::query(dbm::statement() << "DROP DATABASE IF EXISTS " << db_name);
//    setup_session.session::query(dbm::statement() << "CREATE DATABASE " << db_name);
//    setup_session.close();

    // TODO: this is not portable
    system((dbm::statement() << "mysql -u " << db_sett.mysql_username
                             << " -h " << db_sett.mysql_host
                             << " -p" << db_sett.mysql_password
                             << " -e 'DROP DATABASE IF EXISTS " << db_name << "'").get().c_str());
    system((dbm::statement() << "mysql -u " << db_sett.mysql_username
                             << " -h " << db_sett.mysql_host
                             << " -p" << db_sett.mysql_password
                             << " -e 'CREATE DATABASE " << db_name << "'").get().c_str());

    pool.set_max_connections(10);
    pool.set_session_initializer([]() {
        auto conn = std::make_shared<dbm::mysql_session>();
        db_settings::instance().init_mysql_session(*conn, db_name);
        conn->open();
        return conn;
    });

    BOOST_TEST(pool.num_connections() == 0);

    // Acquire connection
    auto& conn1 = pool.acquire_connection();
    BOOST_TEST(pool.num_connections() == 1);
    BOOST_TEST(pool.num_active_connections() == 1);
    BOOST_TEST(pool.num_idle_connections() == 0);

    // Simple query connection 1
    auto rows = conn1.select("SELECT 1 as value");
    BOOST_TEST(rows.size() == 1);

    // Acquire another connection (should create a new one)
    auto& conn2 = pool.acquire_connection();
    BOOST_TEST(pool.num_connections() == 2);
    BOOST_TEST(pool.num_active_connections() == 2);
    BOOST_TEST(pool.num_idle_connections() == 0);

    // Simple query connection 2
    rows = conn2.select("SELECT 1 as value");
    BOOST_TEST(rows.size() == 1);

    // Release connections
    pool.release_connection(conn1);
    BOOST_TEST(pool.num_connections() == 2);
    BOOST_TEST(pool.num_active_connections() == 1);
    BOOST_TEST(pool.num_idle_connections() == 1);

    pool.release_connection(conn2);
    BOOST_TEST(pool.num_connections() == 2);
    BOOST_TEST(pool.num_active_connections() == 0);
    BOOST_TEST(pool.num_idle_connections() == 2);

    // Acquire third connection (should reuse existing)
    auto& conn3 = pool.acquire_connection();
    BOOST_TEST(pool.num_connections() == 2);
    BOOST_TEST(pool.num_active_connections() == 1);
    BOOST_TEST(pool.num_idle_connections() == 1);
}
BOOST_AUTO_TEST_SUITE_END()

#endif //#ifdef DBM_MYSQL