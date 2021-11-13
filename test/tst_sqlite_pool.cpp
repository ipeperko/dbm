#ifdef DBM_SQLITE3

#include "dbm/dbm.hpp"
#include "db_settings.h"
#include "common.h"
#include <dbm/drivers/sqlite/sqlite_session.hpp>

using namespace boost::unit_test;
using namespace std::chrono_literals;

namespace {

void setup_pool(dbm::pool& pool)
{
    pool.set_max_connections(1);
    //pool.enable_debbug(true);
    pool.set_session_initializer([]() {
        auto conn = std::make_shared<dbm::sqlite_session>();
        conn->set_db_name(db_settings::instance().test_db_file_name);
        conn->open();
        return conn;
    });
}

} // namespace

BOOST_AUTO_TEST_SUITE(TstSQLitePool)

BOOST_AUTO_TEST_CASE(pool_init)
{
    dbm::pool pool;
    setup_pool(pool);
    BOOST_TEST(pool.num_connections() == 0);
}

BOOST_AUTO_TEST_CASE(pool_acquire_release)
{
    dbm::pool pool;
    setup_pool(pool);
    BOOST_TEST(pool.num_connections() == 0);

    // Acquire connection
    auto conn1 = pool.acquire();
    BOOST_TEST(conn1.is_valid());
    BOOST_TEST(pool.num_connections() == 1);
    BOOST_TEST(pool.num_active_connections() == 1);
    BOOST_TEST(pool.num_idle_connections() == 0);

    // Simple query connection 1
    auto rows = conn1.get().select("SELECT 1");
    BOOST_TEST(rows.size() == 1);

    // Release connections
    conn1.release();
    BOOST_TEST(!conn1.is_valid());
    BOOST_TEST(pool.num_connections() == 1);
    BOOST_TEST(pool.num_active_connections() == 0);
    BOOST_TEST(pool.num_idle_connections() == 1);

    {
        // Acquire another connection (should reuse the existing one)
        auto conn2 = pool.acquire();
        BOOST_TEST(conn2.is_valid());
        BOOST_TEST(pool.num_connections() == 1);
        BOOST_TEST(pool.num_active_connections() == 1);
        BOOST_TEST(pool.num_idle_connections() == 0);

        // Simple query connection 2
        rows = conn2.get().select("SELECT 1");
        BOOST_TEST(rows.size() == 1);
    }

    // conn2 is out of scope, so the connection should be released
    BOOST_TEST(pool.num_connections() == 1);
    BOOST_TEST(pool.num_active_connections() == 0);
    BOOST_TEST(pool.num_idle_connections() == 1);
}

BOOST_AUTO_TEST_CASE(pool_acquire_timeout_exception)
{
    dbm::pool pool;
    setup_pool(pool);
    pool.set_acquire_timeout(2s);
    BOOST_TEST(pool.num_connections() == 0);

    std::thread t1([&] {
        auto conn1 = pool.acquire();
        BOOST_TEST(pool.num_connections() == 1);
        BOOST_TEST(pool.num_active_connections() == 1);
        BOOST_TEST(pool.num_idle_connections() == 0);

        auto rows = conn1.get().select("SELECT 1");
        BOOST_TEST(rows.size() == 1);
        std::this_thread::sleep_for(3s);
        rows.clear();
        rows = conn1.get().select("SELECT 1");
        BOOST_TEST(rows.size() == 1);
    });

    std::this_thread::sleep_for(1s);

    auto tp1 = std::chrono::steady_clock::now();
    BOOST_REQUIRE_THROW(pool.acquire(), std::exception);
    auto tp2 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> dt = tp2 - tp1;
    BOOST_TEST((dt >= 2000ms));
    BOOST_TEST((dt < 2100ms));

    t1.join();
}

BOOST_AUTO_TEST_SUITE_END()

#endif