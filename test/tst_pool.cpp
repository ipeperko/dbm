#ifdef DBM_MYSQL

#include "dbm/dbm.hpp"
#include "db_settings.h"
#ifdef DBM_MYSQL
#include <dbm/drivers/mysql/mysql_session.hpp>
#endif

//#define BOOST_TEST_MODULE dbm_pool
#include <boost/test/unit_test.hpp>
#include <dbm/impl/session.ipp>
#include <atomic>

using namespace boost::unit_test;

namespace{

constexpr const char* db_name = "dbm_pool_unit_test";

void setup_pool(dbm::pool& pool)
{
    pool.set_max_connections(10);
    pool.enable_debbug(true);
    pool.set_session_initializer([]() {
        auto conn = std::make_shared<dbm::mysql_session>();
        db_settings::instance().init_mysql_session(*conn, db_name);
        conn->open();
        return conn;
    });
}
}

BOOST_AUTO_TEST_SUITE(TstPool)

BOOST_AUTO_TEST_CASE(pool_init)
{
    auto& db_sett = db_settings::instance();
//    dbm::mysql_session setup_session;
//    setup_session.init(db_sett.mysql_host, db_sett.mysql_password, db_sett.mysql_password, db_sett.mysql_port, "");
//    setup_session.open();
//    setup_session.session::query(dbm::statement() << "DROP DATABASE IF EXISTS " << db_name);
//    setup_session.session::query(dbm::statement() << "CREATE DATABASE " << db_name);
//    setup_session.close();

    // TODO: this is not portable, seems like mysql driver does not connect if database is null
    system((dbm::statement() << "mysql -u " << db_sett.mysql_username
                             << " -h " << db_sett.mysql_host
                             << " -p" << db_sett.mysql_password
                             << " -e 'DROP DATABASE IF EXISTS " << db_name << "'").get().c_str());
    system((dbm::statement() << "mysql -u " << db_sett.mysql_username
                             << " -h " << db_sett.mysql_host
                             << " -p" << db_sett.mysql_password
                             << " -e 'CREATE DATABASE " << db_name << "'").get().c_str());

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
    BOOST_TEST(pool.num_connections() == 1);
    BOOST_TEST(pool.num_active_connections() == 1);
    BOOST_TEST(pool.num_idle_connections() == 0);

    // Simple query connection 1
    auto rows = conn1.get().select("SELECT 1");
    BOOST_TEST(rows.size() == 1);

    // Acquire another connection (should create a new one)
    auto conn2 = pool.acquire();
    BOOST_TEST(pool.num_connections() == 2);
    BOOST_TEST(pool.num_active_connections() == 2);
    BOOST_TEST(pool.num_idle_connections() == 0);

    // Simple query connection 2
    rows = conn2.get().select("SELECT 1");
    BOOST_TEST(rows.size() == 1);

    // Release connections
    conn1.release();
    BOOST_TEST(pool.num_connections() == 2);
    BOOST_TEST(pool.num_active_connections() == 1);
    BOOST_TEST(pool.num_idle_connections() == 1);

    conn2.release();
    BOOST_TEST(pool.num_connections() == 2);
    BOOST_TEST(pool.num_active_connections() == 0);
    BOOST_TEST(pool.num_idle_connections() == 2);

    {
        // Acquire third connection (should reuse existing)
        auto conn3 = pool.acquire();
        BOOST_TEST(pool.num_connections() == 2);
        BOOST_TEST(pool.num_active_connections() == 1);
        BOOST_TEST(pool.num_idle_connections() == 1);
    }

    // conn3 is out of scope so it should be released
    BOOST_TEST(pool.num_connections() == 2);
    BOOST_TEST(pool.num_active_connections() == 0);
    BOOST_TEST(pool.num_idle_connections() == 2);
}

BOOST_AUTO_TEST_CASE(pool_connection_move)
{
    dbm::pool pool;
    setup_pool(pool);
    BOOST_TEST(pool.num_connections() == 0);

    {
        dbm::pool_connection conn1;

        // Acquire connection and move to conn1
        {
            auto conn2 = pool.acquire();
            conn1 = std::move(conn2);

            // conn2 has been moved, should throw exception on get
            BOOST_REQUIRE_THROW(conn2.get(), std::exception);
        }

        BOOST_TEST(pool.num_connections() == 1);
        BOOST_TEST(pool.num_active_connections() == 1);
        BOOST_TEST(pool.num_idle_connections() == 0);

        // Simple query
        auto rows = conn1.get().select("SELECT 1");
        BOOST_TEST(rows.size() == 1);
    }

    // conn1 is out of scope, connection should be released
    BOOST_TEST(pool.num_connections() == 1);
    BOOST_TEST(pool.num_active_connections() == 0);
    BOOST_TEST(pool.num_idle_connections() == 1);
}

BOOST_AUTO_TEST_CASE(pool_acquire_timeout_exception)
{
    dbm::pool pool;
    setup_pool(pool);
    pool.set_max_connections(1);
    pool.set_acquire_timeout(std::chrono::seconds(2));
    BOOST_TEST(pool.num_connections() == 0);

    std::thread t1([&] {
        auto conn1 = pool.acquire();
        BOOST_TEST(pool.num_connections() == 1);
        BOOST_TEST(pool.num_active_connections() == 1);
        BOOST_TEST(pool.num_idle_connections() == 0);

        auto rows = conn1.get().select("SELECT 1");
        BOOST_TEST(rows.size() == 1);
        std::this_thread::sleep_for(std::chrono::seconds(3));
        rows.clear();
        rows = conn1.get().select("SELECT 1");
        BOOST_TEST(rows.size() == 1);
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto tp1 = std::chrono::steady_clock::now();
    BOOST_REQUIRE_THROW(pool.acquire(), std::exception);
    auto tp2 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> dt = tp2 - tp1;
    BOOST_TEST(dt.count() >= 2000);
    BOOST_TEST(dt.count() < 2100);

    t1.join();
}

BOOST_AUTO_TEST_CASE(pool_heartbeat_error)
{
    dbm::pool pool;
    setup_pool(pool);
    pool.set_heartbeat_interval(std::chrono::seconds(1));
    BOOST_TEST(pool.num_connections() == 0);

    auto conn = pool.acquire();
    auto* db = &conn.get();
    conn.release();

    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    BOOST_TEST(pool.heartbeats_count() == 2);

    db->close(); // manually close connection

    // verify connection has been removed
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    BOOST_TEST(pool.num_connections() == 0);
}


class PoolHighLoad
{
public:
    PoolHighLoad()
    {
        setup_pool(pool);
        pool.set_max_connections(n_conn);
        pool.set_acquire_timeout(std::chrono::seconds(5));
        BOOST_TEST(pool.num_connections() == 0);
    }

    void run()
    {
        std::atomic<size_t> num_exceptions = 0;
        std::mutex mtx;
        std::atomic<size_t> max_active_conn = 0;
        std::atomic<size_t> total_rows = 0;

        auto update_stat = [&](size_t num_active_conn) {
          std::lock_guard guard(mtx);
          if (num_active_conn > max_active_conn)
              max_active_conn = num_active_conn;
        };

        auto task = [&] {
          try {
              auto conn = pool.acquire();
              auto rows = conn.get().select("SELECT 1");
              total_rows += rows.size();
              BOOST_TEST(rows.size() == 1);

              auto num_active_connections = pool.num_connections();
              update_stat(num_active_connections);

              std::this_thread::sleep_for(std::chrono::milliseconds(100));
          }
          catch(std::exception& e) {
              ++num_exceptions;
              BOOST_TEST_MESSAGE("Exception : " << e.what());
          }
        };

        std::thread threads[n_threads];

        for (auto& t : threads) {
            t = std::thread(task);
        }

        for (auto& t : threads) {
            t.join();
        }

        BOOST_TEST(num_exceptions == 0);
        BOOST_TEST(total_rows == n_threads);
        BOOST_TEST(max_active_conn == pool.max_connections());
        BOOST_TEST(pool.num_idle_connections() == n_conn);
    }

    size_t n_conn = 10;
    size_t n_threads = 300;
    dbm::pool pool;
};

// The goal of this test is to verify if all tasks acquired connection
// in a high load scenario (num threads > max connections in pool)
BOOST_AUTO_TEST_CASE(pool_acquire_high_load)
{
    PoolHighLoad high_load;

    int n_runs = 1;
    int argc = framework::master_test_suite().argc;
    for (int i = 0; i < argc - 1; i++) {
        std::string arg = framework::master_test_suite().argv[i];
        if (arg == "--pool-high-load-runs") {
            n_runs = std::stoi(framework::master_test_suite().argv[i + 1]);
            break;
        }
    }

    for (int i = 0; i < n_runs; i++) {
        high_load.run();

    }
}

BOOST_AUTO_TEST_CASE(pool_variable_load)
{

}

BOOST_AUTO_TEST_SUITE_END()

#endif //#ifdef DBM_MYSQL