#include "dbm/dbm.hpp"
#include "db_settings.h"
#include "common.h"

using namespace boost::unit_test;
using namespace std::chrono_literals;

namespace {

struct Data
{
    int id;
    std::string value;
};

auto get_model(Data& data)
{
    return dbm::model {
        "test_transaction",
        {
            { dbm::key("id"), dbm::binding(data.id), dbm::primary(true), dbm::not_null(true), dbm::auto_increment(true) },
            { dbm::key("value"), dbm::binding(data.value) }
        }
    };
}

template <class DBType>
class Test
{
public:

    void transaction_simple()
    {
        auto db = get_db_session();

        Data data = {};
        auto model = get_model(data);
        model.drop_table(*db);
        model.create_table(*db);

        dbm::session::transaction tr(*db);
        data.id = 1;
        data.value = "first entry";
        model >> *db;
        tr.commit();

        data.value = "";
        *db >> model;
        BOOST_TEST(data.value == "first entry");
    }

    void transaction_rollback()
    {
        auto db = get_db_session();

        Data data = {};
        auto model = get_model(data);

        {
            dbm::session::transaction tr(*db);
            db->query("INSERT INTO test_transaction (value) VALUES('should not be there after rollback')");
            // we didn't commit, rollback expected
        }

        auto count = db->select("SELECT count(*) FROM test_transaction").at(0).at(0).template get<int>();
        BOOST_TEST(count == 1);
    }

    void transaction_parallel()
    {
        auto db = get_db_session();

        Data data = {};
        auto model = get_model(data);
        model.at(0).get().set_defined(false);

        auto thr = std::thread([this]{
            auto s = get_db_session();
            std::this_thread::sleep_for(500ms);
            auto max_id = s->select("SELECT MAX(id) AS max FROM test_transaction").at(0).at(0).template get<std::string>();
            // #4
            s->query("INSERT INTO test_transaction (value) VALUES('parallel max_id was " + max_id + "')");
        });

        dbm::session::transaction tr(*db);
        // #3
        db->query("INSERT INTO test_transaction (value) VALUES('main thread')");
        std::this_thread::sleep_for(1s);
        auto max_id = db->select("SELECT MAX(id) AS max FROM test_transaction").at(0).at(0).template get<std::string>();
        data.value = "main max_id was " + max_id;
        // #5
        model >> *db;
        tr.commit();

        thr.join();

        model.at(0).get().set_defined(true);

        data = {};
        data.id = 4;
        *db >> model;
        // Parallel thread inserted entry id 3 before the parallel thread,
        // but the select max_id was called after the parallel thread.
        // Parallel max id query result should be 1 (without transaction the max id value is 3).
        BOOST_TEST(data.value == "parallel max_id was 1");

        data = {};
        data.id = 5;
        *db >> model;
        BOOST_TEST(data.value == "main max_id was 4");
    }

    void run_tests()
    {
        transaction_simple();
        transaction_rollback();
        if constexpr (std::is_same_v<DBType, dbm::mysql_session>) {
            transaction_parallel();
        }
    }

    std::unique_ptr<dbm::session> get_db_session()
    {
        if constexpr (std::is_same_v<DBType, dbm::mysql_session>) {
            return db_settings::instance().get_mysql_session();
        }
        else if constexpr (std::is_same_v<DBType, dbm::sqlite_session>) {
            return db_settings::instance().get_sqlite_session();
        }
        return nullptr;
    }
};

} // namespace

BOOST_AUTO_TEST_SUITE(TstTransaction)

BOOST_AUTO_TEST_CASE(run_transactions)
{
#ifdef DBM_MYSQL
    BOOST_TEST_CHECKPOINT("transactions test MySql");
    Test<dbm::mysql_session>().run_tests();
#endif
#ifdef DBM_SQLITE3
    BOOST_TEST_CHECKPOINT("transactions test SQLite");
    Test<dbm::sqlite_session>().run_tests();
#endif
}

BOOST_AUTO_TEST_SUITE_END()