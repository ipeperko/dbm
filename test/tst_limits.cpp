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

struct Max
{
    template<typename T>
    static constexpr void set(T& val)
    {
        val = std::numeric_limits<T>::max();
    }

    template<typename T>
    static constexpr T get()
    {
        return std::numeric_limits<T>::max();
    }
};

struct Min
{
    template<typename T>
    static constexpr void set(T& val)
    {
        val = std::numeric_limits<T>::min();
    }

    template<typename T>
    static constexpr T get()
    {
        return std::numeric_limits<T>::min();
    }
};


template<typename MinMax, size_t Index=0>
void set_values(values_t& t)
{
    using SelectedType = std::tuple_element_t<Index, values_t>;

    MinMax::template set<SelectedType>(std::get<Index>(t));

    if constexpr (Index < std::tuple_size_v<values_t> - 1) {
        set_values<MinMax, Index + 1>(t);
    }
}

template<typename MinMax, typename FilterType=void, size_t Index=0>
void check_values(values_t& t)
{
    using SelectedType = std::tuple_element_t<Index, values_t>;

    if constexpr (not std::is_same_v<SelectedType, FilterType>) {
        BOOST_TEST(std::get<Index>(t) == MinMax::template get<SelectedType>());
    }

    if constexpr (Index < std::tuple_size_v<values_t> - 1) {
        check_values<MinMax, FilterType, Index + 1>(t);
    }
}

template<size_t Index=0>
void from_container_vector(values_t& t, std::vector<std::unique_ptr<dbm::container>> const& v)
{
    using SelectedType = std::tuple_element_t<Index, values_t>;

    if (std::tuple_size_v<values_t> + 1 != v.size())
        throw std::domain_error("vector size error");

    std::get<Index>(t) = v[Index + 1]->get<SelectedType>();

    if constexpr (Index < std::tuple_size_v<values_t> - 1) {
        from_container_vector<Index + 1>(t, v);
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

template<typename DBType>
void init(DBType& db)
{
    values_t lm {};
    auto m = get_model(lm, "init");
    m.drop_table(db);
    m.create_table(db);
}

template<typename DBType>
void insert_model_limits(DBType& db)
{
    values_t lm {};
    set_values<Max>(lm);
    auto m = get_model(lm, "model max");
    m >> db;

    set_values<Min>(lm);
    m.at("descr").set_value("model min");
    m >> db;
}

template<typename DBType, typename MinMax, typename Filter=void>
void check_model_limits(DBType& db, std::string const& descr)
{
    values_t lm {};
    auto m = get_model(lm, descr);
    db >> m;

    check_values<MinMax, Filter>(lm);
}

template<typename DBType>
void insert_prepared_stmt_limits(DBType& db)
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

    set_values<Max>(lm);
    stmt >> db;

    stmt.param(0)->set("stmt min");
    set_values<Min>(lm);
    stmt >> db;
}

template<typename DBType, typename MinMax, typename Filter=void>
void check_prepared_stmt_limits(DBType& db)
{
    std::string descr;
    if constexpr (std::is_same_v<MinMax, Max>)
        descr = "stmt max";
    else if constexpr (std::is_same_v<MinMax, Min>)
        descr = "stmt min";

    dbm::prepared_stmt stmt("SELECT * FROM test_limits WHERE descr='" + descr + "'",
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

    values_t lm {};
    from_container_vector(lm, row);

    check_values<MinMax, Filter>(lm);
}


// TODO: put all functions in a class with template parameter DBType
template<typename DBType, typename Filter=void>
void check_limits_impl(DBType& db)
{
    init(db);

    insert_model_limits(db);
    BOOST_TEST_CHECKPOINT("model max limits");
    check_model_limits<DBType, Max, Filter>(db, "model max");
    BOOST_TEST_CHECKPOINT("model min limits");
    check_model_limits<DBType, Min>(db, "model min");

    insert_prepared_stmt_limits(db);
    BOOST_TEST_CHECKPOINT("prepared statement max limits");
    check_prepared_stmt_limits<DBType, Max, Filter>(db);

    BOOST_TEST_CHECKPOINT("prepared statement min limits");
    check_prepared_stmt_limits<DBType, Min>(db);
}

} // namespace

BOOST_AUTO_TEST_SUITE(TstLimits)

BOOST_AUTO_TEST_CASE(check_limits, * tolerance(0.00001))
{
#ifdef DBM_MYSQL
    BOOST_TEST_CHECKPOINT("check limits MySql");
    check_limits_impl<dbm::mysql_session>(*db_settings::instance().get_mysql_session());
#endif
#ifdef DBM_SQLITE3
    BOOST_TEST_CHECKPOINT("check limits SQLite");
    // uint64 will be filtered out from checks
    check_limits_impl<dbm::sqlite_session, uint64_t>(*db_settings::instance().get_sqlite_session());
#endif
}

BOOST_AUTO_TEST_SUITE_END()

