#include "dbm/dbm.hpp"
#include "db_settings.h"
#include "common.h"

#include <tuple>

using namespace boost::unit_test;

namespace {

using values_t =
    std::tuple<
        bool,
        int32_t,
        int16_t,
        int64_t,
        uint32_t,
        uint16_t,
        uint64_t,
        double>;

struct SetMax
{
    template<typename T>
    explicit SetMax(T& val)
    {
        val = std::numeric_limits<T>::max();
    }
};

struct SetMin
{
    template<typename T>
    explicit SetMin(T& val)
    {
        val = std::numeric_limits<T>::min();
    }
};

template<typename MinMax, size_t Index=0>
void set_values(values_t& t)
{
    MinMax(std::get<Index>(t));
    if constexpr (Index < std::tuple_size_v<values_t> - 1) {
        set_values<MinMax, Index + 1>(t);
    }
}

struct Max
{
    template<typename T>
    static constexpr T get()
    {
        return std::numeric_limits<T>::max();
    }
};

struct Min
{
    template<typename T>
    static constexpr T get()
    {
        return std::numeric_limits<T>::min();
    }
};

template<typename MinMax, typename FilterType=void, size_t Index=0>
void
check_values(values_t& t)
{
    using SelectedType = std::tuple_element_t<Index, values_t>;

    if constexpr (not std::is_same_v<SelectedType, FilterType>) {
        BOOST_TEST(std::get<Index>(t) == MinMax::template get<SelectedType>());
    }

    if constexpr (Index < std::tuple_size_v<values_t> - 1) {
        check_values<MinMax, FilterType, Index + 1>(t);
    }
}

auto get_model(values_t& lm, std::string descr)
{
    return dbm::model {
        "test_limits",
        {
          { dbm::key("descr"), dbm::local<std::string>(std::move(descr)), dbm::primary(true), dbm::not_null(true), dbm::custom_data_type("VARCHAR(64)") },
          { dbm::key("bool"), dbm::binding(std::get<bool>(lm)) },
          { dbm::key("int32_t"), dbm::binding(std::get<int32_t>(lm)) },
          { dbm::key("int16_t"), dbm::binding(std::get<int16_t>(lm)) },
          { dbm::key("int64_t"), dbm::binding(std::get<int64_t>(lm)) },
          { dbm::key("uint32_t"), dbm::binding(std::get<uint32_t>(lm)) },
          { dbm::key("uint16_t"), dbm::binding(std::get<uint16_t>(lm)) },
          { dbm::key("uint64_t"), dbm::binding(std::get<uint64_t>(lm)) },
          { dbm::key("double_"), dbm::binding(std::get<double>(lm)) }
        }
    };
}

void init(dbm::session& db)
{
    values_t lm {};
    auto m = get_model(lm, "init");
    m.drop_table(db);
    m.create_table(db);
}

void insert_model_limits(dbm::session& db)
{
    values_t lm {};
    set_values<SetMax>(lm);
    auto m = get_model(lm, "model max");
    m >> db;

    set_values<SetMin>(lm);
    m.at("descr").set_value("model min");
    m >> db;
}

template<typename MinMax, typename Filter=void>
void check_model_limits(dbm::session& db, std::string const& descr)
{
    values_t lm {};
    auto m = get_model(lm, descr);
    db >> m;

    check_values<MinMax, Filter>(lm);
}

void insert_prepared_stmt_limits(dbm::session& db)
{
    values_t lm {};

    dbm::prepared_stmt stmt("INSERT INTO test_limits VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)",
                       dbm::local<std::string>("stmt max"),
                       dbm::binding(std::get<bool>(lm)),
                       dbm::binding(std::get<int32_t>(lm)),
                       dbm::binding(std::get<int16_t>(lm)),
                       dbm::binding(std::get<int64_t>(lm)),
                       dbm::binding(std::get<uint32_t>(lm)),
                       dbm::binding(std::get<uint16_t>(lm)),
                       dbm::binding(std::get<uint64_t>(lm)),
                       dbm::binding(std::get<double>(lm))
                       );

    set_values<SetMax>(lm);
    stmt >> db;

    stmt.param(0)->set("stmt min");
    set_values<SetMin>(lm);
    stmt >> db;
}


void check_prepared_stmt_limits_max(dbm::session& db, bool check_uint64)
{
    dbm::prepared_stmt stmt("SELECT * FROM test_limits WHERE descr='stmt max'",
                            dbm::local<std::string>(),
                            dbm::local<bool>(),
                            dbm::local<int32_t>(),
                            dbm::local<int16_t>(),
                            dbm::local<int64_t>(),
                            dbm::local<uint32_t>(),
                            dbm::local<uint16_t>(),
                            dbm::local<uint64_t>(),
                            dbm::local<double>()
                            );

    auto rows = db.select(stmt);
    auto& row = rows.at(0);

    BOOST_TEST(row.at(1)->get<bool>() == std::numeric_limits<bool>::max());
    BOOST_TEST(row.at(2)->get<int32_t>() == std::numeric_limits<int32_t>::max());
    BOOST_TEST(row.at(3)->get<int16_t>() == std::numeric_limits<int16_t>::max());
    BOOST_TEST(row.at(4)->get<int64_t>() == std::numeric_limits<int64_t>::max());
    BOOST_TEST(row.at(5)->get<uint32_t>() == std::numeric_limits<uint32_t>::max());
    BOOST_TEST(row.at(6)->get<uint16_t>() == std::numeric_limits<uint16_t>::max());
    if (check_uint64) {
        BOOST_TEST(row.at(7)->get<uint64_t>() == std::numeric_limits<uint64_t>::max());
    }
    BOOST_TEST(row.at(8)->get<double>() == std::numeric_limits<double>::max());
}

void check_prepared_stmt_limits_min(dbm::session& db)
{
    dbm::prepared_stmt stmt("SELECT * FROM test_limits WHERE descr='stmt min'",
                            dbm::local<std::string>(),
                            dbm::local<bool>(),
                            dbm::local<int32_t>(),
                            dbm::local<int16_t>(),
                            dbm::local<int64_t>(),
                            dbm::local<uint32_t>(),
                            dbm::local<uint16_t>(),
                            dbm::local<uint64_t>(),
                            dbm::local<double>()
    );

    auto rows = db.select(stmt);
    auto& row = rows.at(0);

    BOOST_TEST(row.at(1)->get<bool>() == std::numeric_limits<bool>::min());
    BOOST_TEST(row.at(2)->get<int32_t>() == std::numeric_limits<int32_t>::min());
    BOOST_TEST(row.at(3)->get<int16_t>() == std::numeric_limits<int16_t>::min());
    BOOST_TEST(row.at(4)->get<int64_t>() == std::numeric_limits<int64_t>::min());
    BOOST_TEST(row.at(5)->get<uint32_t>() == std::numeric_limits<uint32_t>::min());
    BOOST_TEST(row.at(6)->get<uint16_t>() == std::numeric_limits<uint16_t>::min());
    BOOST_TEST(row.at(7)->get<uint64_t>() == std::numeric_limits<uint64_t>::min());
    BOOST_TEST(row.at(8)->get<double>() == std::numeric_limits<double>::min());
}

void check_limits_impl(dbm::session& db, bool check_uint64=true)
{
    init(db);

    insert_model_limits(db);
    BOOST_TEST_CHECKPOINT("model max limits");
    if (check_uint64)
        check_model_limits<Max>(db, "model max");
    else
        check_model_limits<Max, uint64_t>(db, "model max");
    BOOST_TEST_CHECKPOINT("model min limits");
    check_model_limits<Min>(db, "model min");

    insert_prepared_stmt_limits(db);
    BOOST_TEST_CHECKPOINT("prepared statement max limits");
    check_prepared_stmt_limits_max(db, check_uint64);
    BOOST_TEST_CHECKPOINT("prepared statement min limits");
    check_prepared_stmt_limits_min(db);
}

} // namespace

BOOST_AUTO_TEST_SUITE(TstLimits)

BOOST_AUTO_TEST_CASE(check_limits, * tolerance(0.00001))
{
#ifdef DBM_MYSQL
    BOOST_TEST_CHECKPOINT("check limits MySql");
    check_limits_impl(*db_settings::instance().get_mysql_session());
#endif
#ifdef DBM_SQLITE3
    BOOST_TEST_CHECKPOINT("check limits SQLite");
    // uint64 will be filtered out from checks
    check_limits_impl(*db_settings::instance().get_sqlite_session(), false);
#endif
}

BOOST_AUTO_TEST_SUITE_END()

