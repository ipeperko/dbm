#ifdef DBM_MYSQL

#include "db_settings.h"
#include "common.h"
#include "dbm/dbm.hpp"
#include <dbm/drivers/mysql/mysql_session.hpp>
#include <boost/range/irange.hpp>
#include <random>

using namespace boost::unit_test;

namespace{

constexpr std::string_view table_name = "tst_mysql_pool_multi";
constexpr unsigned n_inserts_per_iteration = 100;
constexpr unsigned n_iterations = 2;
std::mutex mtx_readers;
std::list<std::thread> reader_threads;
constexpr unsigned thread_sleep_time = 20;

class TstPool : public dbm::pool
{
    TstPool()
    {
        setup_pool();
        create_schema();
    }
public:
    static TstPool& instance()
    {
        static TstPool pool;
        return pool;
    }

private:
    void setup_pool()
    {
        set_max_connections(10);
        //pool.enable_debbug(true);
        set_session_initializer([]() {
            auto conn = std::make_shared<dbm::mysql_session>();
            db_settings::instance().init_mysql_session(*conn, db_settings::instance().test_db_name);
            conn->open();
            return conn;
        });
    }

    void create_schema()
    {
        dbm::model m(table_name.data(),
                   {
                       { dbm::key("id"), dbm::local<unsigned>(), dbm::primary(true), dbm::auto_increment(true), dbm::not_null(true) },
                       { dbm::key("thread_id"), dbm::local<unsigned>(), dbm::primary(true) },
                       { dbm::key("iteration"), dbm::local<unsigned>(), dbm::primary(true) },
                       { dbm::key("value"), dbm::local<double>() }
                   });

        auto con = acquire();
        m.drop_table(*con);
        m.create_table(*con);

        con.get().query("DROP FUNCTION IF EXISTS insert_tst_mysql_pool_multi");
        con.get().query(R"(CREATE FUNCTION insert_tst_mysql_pool_multi(_id INT, _iter INT, _val DOUBLE)
RETURNS INT DETERMINISTIC
BEGIN
        INSERT INTO tst_mysql_pool_multi
        (thread_id, iteration, value)
        VALUES (_id, _iter, _val);
        RETURN 1;
END;)");
    }
};

class LaunchReaders
{
public:
    LaunchReaders(unsigned writer_id, unsigned iter_id)
    {
        run([writer_id, iter_id] {
            auto con = TstPool::instance().acquire();
            BOOST_TEST(con);
            auto rows = con.get().select(dbm::statement() << "SELECT * FROM " << std::string(table_name) <<
                             " WHERE thread_id=" << writer_id << " AND iteration=" << iter_id);
            BOOST_TEST(rows.size() == n_inserts_per_iteration);
        });

        run([writer_id, iter_id] {
            auto con = TstPool::instance().acquire();
            BOOST_TEST(con);
            auto rows = con.get().select(dbm::statement() << "SELECT COUNT(*) AS count FROM " << std::string(table_name) <<
                             " WHERE thread_id=" << writer_id << " AND iteration=" << iter_id);
            BOOST_TEST(rows.size() == 1);
            BOOST_TEST(rows.front().at("count").get<int>() == n_inserts_per_iteration);
        });

        run([writer_id, iter_id] {
            auto con = TstPool::instance().acquire();
            BOOST_TEST(con);
            dbm::prepared_stmt stmt("SELECT * FROM " + std::string(table_name) + " WHERE thread_id=" +
                                        std::to_string(writer_id) + " AND iteration=" + std::to_string(iter_id),
                                    dbm::local<int>(),
                                    dbm::local<int>(),
                                    dbm::local<int>(),
                                    dbm::local<double>() );
            auto res = con.get().select(stmt);
            BOOST_TEST(res.size() == n_inserts_per_iteration);
        });
    }

private:
    template<typename Func>
    void run(Func&& fn)
    {
        auto thr = std::thread([f = std::forward<Func>(fn)] {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(0, thread_sleep_time);
            std::this_thread::sleep_for(std::chrono::milliseconds(distrib(gen)));

            f();
        });

        std::lock_guard lock(mtx_readers);
        reader_threads.emplace_back(std::move(thr));
    }
};

class WriteTask
{
public:
    WriteTask(unsigned id)
        : id_(id)
    {
        switch(id_) {
            case 0: writer_ = std::bind(&WriteTask::writer_with_model, this); break;
            case 1: writer_ = std::bind(&WriteTask::writer_pure_insert, this); break;
            case 2: writer_ = std::bind(&WriteTask::writer_prepared_stmt, this); break;
            case 3: writer_ = std::bind(&WriteTask::writer_function, this); break;
            default:;
        }
        BOOST_TEST((writer_ != nullptr));
    }

    void run()
    {
        if (thr_.joinable())
            thr_.join();

        thr_ = std::thread([this] {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(0, thread_sleep_time);

            auto n = iter_ + n_iterations;

            for (; iter_ < n; ++iter_) {
                for (auto i = 0u; i < n_inserts_per_iteration; ++i) {
                    writer_();
                    std::this_thread::sleep_for(std::chrono::milliseconds(distrib(gen)));
                }
                LaunchReaders(id_, iter_);
            }
        });
    }

    void join()
    {
        if (thr_.joinable())
            thr_.join();
    }

private:
    void writer_with_model()
    {
        dbm::model m("tst_mysql_pool_multi",
                     {
                         { dbm::key("thread_id"), dbm::binding(id_) },
                         { dbm::key("iteration"), dbm::binding(iter_) },
                         { dbm::key("value"), dbm::local<double>(1.0) }
                     });

        auto con = TstPool::instance().acquire();
        BOOST_TEST(con);
        m >> *con;
    }

    void writer_pure_insert()
    {
        auto con = TstPool::instance().acquire();
        BOOST_TEST(con);
        con.get().query(dbm::statement() << "INSERT INTO " << table_name << " VALUES(NULL, " << id_ << ", " << iter_ << ", " << "2.2)");
    }

    void writer_prepared_stmt()
    {
        auto con = TstPool::instance().acquire();
        BOOST_TEST(con);
        dbm::prepared_stmt stmt("INSERT INTO " + std::string(table_name) + " VALUES(NULL, ?, ?, ?)",
                                dbm::binding(id_),
                                dbm::binding(iter_),
                                dbm::local(3.3));
        con.get().query(stmt);
    }

    void writer_function()
    {
        auto con = TstPool::instance().acquire();
        BOOST_TEST(con);
        dbm::prepared_stmt stmt("SELECT insert_tst_mysql_pool_multi(?, ?, ?)",
                               dbm::binding(id_),
                               dbm::binding(iter_),
                               dbm::local(3.3));
        con.get().query(stmt);
    }

    unsigned id_ {0};
    unsigned iter_ {0};
    std::thread thr_;
    std::function<void()> writer_;
};

} // namespace

BOOST_AUTO_TEST_SUITE(TstMysqlPoolMulti)

BOOST_AUTO_TEST_CASE(pool_init)
{
    auto& pool = TstPool::instance();
    BOOST_TEST(pool.num_connections() == 1);
}

BOOST_AUTO_TEST_CASE(pool_write_and_read_multi)
{
    std::list<WriteTask> tasks;

    for (auto i : boost::irange(0, 4))
        tasks.emplace_back(i);

    for (auto i : boost::irange(0, 4)) {
        std::for_each(tasks.begin(), tasks.end(), [](WriteTask& t) {
            t.run();
        });

        std::for_each(tasks.begin(), tasks.end(), [](WriteTask& t) {
            t.join();
        });
    }

    std::lock_guard lock(mtx_readers);
    std::for_each(reader_threads.begin(), reader_threads.end(), [](std::thread& t) {
        t.join();
    });

    std::cout << "num connections " << TstPool::instance().num_connections() << "\n";
}

BOOST_AUTO_TEST_SUITE_END()

#endif