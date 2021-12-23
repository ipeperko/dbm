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
    pool.set_session_initializer([]() {
        auto conn = std::make_shared<dbm::sqlite_session>();
        db_settings::instance().init_sqlite_session(*conn);
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

class MultipleWriters
{
    dbm::pool pool_;
    static constexpr unsigned n_tasks_ {10};
    static constexpr unsigned n_rec_ {100};
public:

    MultipleWriters()
    {
        setup_pool(pool_);
        pool_.set_acquire_timeout(20s);

        auto session = pool_.acquire();
        auto m = get_model();
        m.drop_table(*session);
        m.create_table(*session);
    }

    dbm::model get_model()
    {
        return dbm::model ("test_pool",
                     {
                         { dbm::key("id"), dbm::local<int>(), dbm::primary(true), dbm::not_null(true) },
                         { dbm::key("thread_id"), dbm::local<int>() },
                         { dbm::key("value"), dbm::local<int>() }
                     });
    }

    void run()
    {
        std::array<std::thread, n_tasks_> thr;

        for (auto i = 0u; i < n_tasks_; ++i) {
            thr[i] = std::thread([this, i]{ inserter(i); });
        }

        for (auto& it : thr) {
            it.join();
        }

        check_inserted();
    }

    void inserter(unsigned thread_id)
    {
        //std::cout << "Inserter start " << thread_id << std::endl;
        auto session = pool_.acquire();
        //std::cout << "Inserter acquired " << thread_id << std::endl;
        dbm::prepared_stmt stmt("INSERT INTO test_pool (thread_id, value) VALUES (?, ?)",
                                dbm::local(thread_id),
                                dbm::local<unsigned>());

        for (auto i = thread_id * n_rec_; i < (thread_id + 1) * n_rec_; ++i) {
            stmt.param(1)->set(i + 1);
            stmt >> *session;
        }
        //std::cout << "Inserter finish " << thread_id << std::endl;
    }

    void check_inserted()
    {
        //std::cout << "Reader start" << std::endl;
        auto session = pool_.acquire();
        //std::cout << "Reader acquired" << std::endl;
        auto n = session.get().select("SELECT COUNT(*) FROM test_pool").at(0).at(0).get<unsigned>();
        BOOST_TEST(n == n_tasks_ * n_rec_);
        //std::cout << "Reader finish" << std::endl;
    }
};

BOOST_AUTO_TEST_CASE(pool_multiple_threads)
{
    MultipleWriters test;
    test.run();
}

BOOST_AUTO_TEST_SUITE_END()

#endif