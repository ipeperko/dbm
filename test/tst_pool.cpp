#ifdef DBM_MYSQL

#include "dbm/dbm.hpp"
#include "db_settings.h"
#include "common.h"
#ifdef DBM_MYSQL
#include <dbm/drivers/mysql/mysql_session.hpp>
#endif
#include <atomic>

using namespace boost::unit_test;
using namespace std::chrono_literals;

namespace{
void setup_pool(MySqlPool& pool)
{
    pool.set_max_connections(10);
}
}

BOOST_AUTO_TEST_SUITE(TstPool)

BOOST_AUTO_TEST_CASE(pool_init)
{
    MySqlPool pool;
    setup_pool(pool);
    BOOST_TEST(pool.num_connections() == 0);
}

BOOST_AUTO_TEST_CASE(pool_acquire_release)
{
    MySqlPool pool;
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

    // Acquire another connection (should create a new one)
    auto conn2 = pool.acquire();
    BOOST_TEST(conn2.is_valid());
    BOOST_TEST(pool.num_connections() == 2);
    BOOST_TEST(pool.num_active_connections() == 2);
    BOOST_TEST(pool.num_idle_connections() == 0);

    // Simple query connection 2
    rows = conn2.get().select("SELECT 1");
    BOOST_TEST(rows.size() == 1);

    // Release connections
    conn1.release();
    BOOST_TEST(!conn1.is_valid());
    BOOST_TEST(pool.num_connections() == 2);
    BOOST_TEST(pool.num_active_connections() == 1);
    BOOST_TEST(pool.num_idle_connections() == 1);

    conn2.release();
    BOOST_TEST(!conn2.is_valid());
    BOOST_TEST(pool.num_connections() == 2);
    BOOST_TEST(pool.num_active_connections() == 0);
    BOOST_TEST(pool.num_idle_connections() == 2);

    {
        // Acquire third connection (should reuse an existing one)
        auto conn3 = pool.acquire();
        BOOST_TEST(pool.num_connections() == 2);
        BOOST_TEST(pool.num_active_connections() == 1);
        BOOST_TEST(pool.num_idle_connections() == 1);
    }

    // conn3 is out of scope, so the connection should be released
    BOOST_TEST(pool.num_connections() == 2);
    BOOST_TEST(pool.num_active_connections() == 0);
    BOOST_TEST(pool.num_idle_connections() == 2);
}

BOOST_AUTO_TEST_CASE(pool_connection_move)
{
    MySqlPool pool;
    setup_pool(pool);
    BOOST_TEST(pool.num_connections() == 0);

    {
        MySqlPool::pool_connection_type conn1(pool, nullptr);

        // Acquire connection and move to conn1
        {
            auto conn2 = pool.acquire();
            BOOST_TEST(pool.num_connections() == 1);
            BOOST_TEST(pool.num_active_connections() == 1);
            conn1 = std::move(conn2);

            // conn2 has been moved, should throw exception on get
            BOOST_REQUIRE_THROW(*conn2, std::exception);
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

BOOST_AUTO_TEST_CASE(pool_connection_not_allow_move_across_pools)
{
    MySqlPool pool1, pool2;
    setup_pool(pool1);
    setup_pool(pool2);

    BOOST_TEST(pool1.num_connections() == 0);
    BOOST_TEST(pool2.num_connections() == 0);

    auto conn1 = pool1.acquire();
    BOOST_TEST(pool1.num_connections() == 1);
    BOOST_TEST(pool2.num_connections() == 0);

    MySqlPool::pool_connection_type conn2(pool2, nullptr);
    conn2 = std::move(conn1);
    BOOST_TEST(pool1.num_connections() == 1);
    BOOST_TEST(pool2.num_connections() == 0);
}

BOOST_AUTO_TEST_CASE(pool_acquire_timeout_exception)
{
    MySqlPool pool;
    setup_pool(pool);
    pool.set_max_connections(1);
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

BOOST_AUTO_TEST_CASE(pool_heartbeat_error)
{
    MySqlPool pool;
    setup_pool(pool);
    pool.set_heartbeat_interval(1s);
    BOOST_TEST(pool.num_connections() == 0);

    auto conn = pool.acquire();
    auto* db = &conn.get();
    conn.release();

    std::this_thread::sleep_for(2800ms);
    BOOST_TEST(pool.heartbeats_count() >= 1); // expected 2 but may be less on slower systems
    BOOST_TEST(pool.heartbeats_count() <= 2);

    db->close(); // manually close connection

    // verify connection has been removed
    std::this_thread::sleep_for(1500ms);
    BOOST_TEST(pool.num_connections() == 0);
}

BOOST_AUTO_TEST_CASE(pool_no_handover_if_connection_closed)
{
    MySqlPool pool;
    setup_pool(pool);
    pool.set_max_connections(1);
    pool.set_heartbeat_interval(1s);

    {
        auto conn = pool.acquire();
        BOOST_TEST(pool.num_connections() == 1);
        conn.get().close();
    }

    // when a closed connection is released it should not be moved to the idle session list
    BOOST_TEST(pool.num_connections() == 0);
}

BOOST_AUTO_TEST_CASE(pool_acquire_order)
{
    MySqlPool pool;
    size_t const pool_size = 5;
    size_t const nconn = 100;

    //dbm::utils::debug_logger::writer = dbm::utils::debug_logger::stdout_writer;

    setup_pool(pool);
    pool.set_heartbeat_interval(0s);
    pool.set_max_connections(pool_size);
    pool.set_acquire_timeout(15s);

    std::vector<MySqlPool::pool_connection_type> conn_active;
    conn_active.reserve(pool_size);

    std::vector<std::thread> thr_waiting;
    thr_waiting.reserve(nconn);
    std::vector<MySqlPool::pool_connection_type> conn_waiting;
    conn_waiting.reserve(nconn);

    for (auto i = 0u; i < pool_size; i++) {
        conn_active.emplace_back(pool.acquire());
    }

    BOOST_TEST(pool.num_active_connections() == pool_size);

    for (auto i = 0u; i < nconn; i++) {
        thr_waiting.emplace_back([&] {
            auto con = pool.acquire();
            std::this_thread::sleep_for(200ms);
        });
    }

    std::this_thread::sleep_for(1s);
    std::mutex mtx;
    std::vector<MySqlPool::id_t> acquired;
    pool.set_event_callback([&](dbm::pool_event event, MySqlPool::id_t id) {
        dbm::utils::debug_logger(dbm::utils::debug_logger::level::Debug) << "pool event #" << id << " " << dbm::to_string(event);
        if (event == dbm::pool_event::acquired) {
            std::lock_guard lck(mtx);
            acquired.push_back(id);
        }
    }, false); // set to blocking event in order to properly check handover order

    conn_active.clear();

    for (auto i = 0u; i < nconn; i++) {
        thr_waiting[i].join();
    }

    BOOST_TEST(acquired.size() == nconn);

    for (auto i = 1u; i < nconn; i++) {
        BOOST_TEST(acquired[i-1] < acquired[i]);
    }
}

class LoadTask
{
public:

    explicit LoadTask(MySqlPool& p)
        : pool(p)
    {}

    void run(size_t n_threads)
    {
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

              std::this_thread::sleep_for(100ms);
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
    }

    void reset()
    {
        num_exceptions = 0;
        max_active_conn = 0;
        total_rows = 0;
    }

    MySqlPool& pool;
    std::atomic<size_t> num_exceptions = 0;
    std::mutex mtx;
    std::atomic<size_t> max_active_conn = 0;
    std::atomic<size_t> total_rows = 0;
};

class PoolHighLoad
{
public:
    PoolHighLoad()
    {
        setup_pool(pool);
        pool.set_max_connections(n_conn);
        pool.set_acquire_timeout(5s);
        BOOST_TEST(pool.num_connections() == 0);
    }

    void run()
    {
        LoadTask task(pool);
        task.run(n_threads);

        BOOST_TEST(task.num_exceptions == 0);
        BOOST_TEST(task.total_rows == n_threads);
        BOOST_TEST(task.max_active_conn == pool.max_connections());
        BOOST_TEST(pool.num_idle_connections() == n_conn);
    }

    size_t const n_conn = 10;
    size_t const n_threads = 300;
    MySqlPool pool;
};

// The goal of this test is to verify if all tasks acquired connection
// in a high load scenario (num threads > max connections in pool)
BOOST_AUTO_TEST_CASE(pool_acquire_high_load)
{
    PoolHighLoad high_load;

    for (int i = 0; i < get_cmlopt_nun_runs(1); i++) {
        high_load.run();
    }
}


class PoolVariableload
{
public:
    PoolVariableload()
    {
        setup_pool(pool);
        pool.set_heartbeat_interval(1s);
        BOOST_TEST(pool.num_connections() == 0);
    }

    void run()
    {
        LoadTask task(pool);
        bool run = true;

        ++run_count;

        auto side_thr = std::thread([&] {
            while (run) {
                std::this_thread::sleep_for(250ms);
                task.pool.num_connections();
                task.pool.num_idle_connections();
                task.pool.num_active_connections();
                task.pool.stat();
            }
        });

        auto reset_counters = [&] {
            task.total_rows = 0;
            task.num_exceptions = 0;
            pool.reset_heartbeats_counter();
        };

        // run 5 threads
        BOOST_TEST_MESSAGE("Pool connecting 5 threads");
        reset_counters();
        task.run(5);
        BOOST_TEST(task.num_exceptions == 0);
        BOOST_TEST(task.total_rows == 5);
        std::this_thread::sleep_for(500ms);

        // run another 5 threads - all connections must be reused
        BOOST_TEST_MESSAGE("Pool connecting 5 threads");
        reset_counters();
        task.run(5);
        BOOST_TEST(task.num_exceptions == 0);
        BOOST_TEST(task.total_rows == 5);
        std::this_thread::sleep_for(500ms);

        // run 8 threads
        BOOST_TEST_MESSAGE("Pool connecting 8 threads");
        reset_counters();
        task.run(8);
        BOOST_TEST(task.num_exceptions == 0);
        BOOST_TEST(task.total_rows == 8);
        std::this_thread::sleep_for(500ms);

        // run 20 threads
        BOOST_TEST_MESSAGE("Pool connecting 20 threads");
        reset_counters();
        task.run(20);
        BOOST_TEST(task.num_exceptions == 0);
        BOOST_TEST(task.total_rows == 20);
        pool.reset_heartbeats_counter();
        std::this_thread::sleep_for(2500ms);
        BOOST_TEST(pool.heartbeats_count() >= 10); // normally 20 but side thread might block the process
        BOOST_TEST(pool.heartbeats_count() <= 30); // normally 20 but sometimes might the process delay while heartbeat still in progress

        // run 5 threads
        BOOST_TEST_MESSAGE("Pool connecting 5 threads");
        reset_counters();
        task.run(5);
        BOOST_TEST(task.num_exceptions == 0);
        BOOST_TEST(task.total_rows == 5);
        pool.reset_heartbeats_counter();
        std::this_thread::sleep_for(2500ms);
        BOOST_TEST(pool.heartbeats_count() >= 10); // normally 20 but side thread might block the process
        BOOST_TEST(pool.heartbeats_count() <= 30); // normally 20 but sometimes might the process delay while heartbeat still in progress

        BOOST_TEST(task.max_active_conn == pool.max_connections());
        BOOST_TEST(pool.num_idle_connections() == pool.max_connections());

        run = false;
        side_thr.join();
    }

    MySqlPool pool;
    size_t run_count {0};
};

BOOST_AUTO_TEST_CASE(pool_variable_load)
{
    PoolVariableload var_load;

    for (int i = 0; i < get_cmlopt_nun_runs(2); i++) {
        var_load.run();
    }
}

BOOST_AUTO_TEST_SUITE_END()

#endif //#ifdef DBM_MYSQL
