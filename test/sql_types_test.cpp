#include "dbm.hpp"
#include <boost/test/unit_test.hpp>
#include <iomanip>

using namespace boost::unit_test;

BOOST_AUTO_TEST_CASE(sql_value_int_test)
{
    dbm::kind::sql_value value("13", strlen("13"));
    std::string s43("43");
    std::string_view sv44("44");

    auto assign_to_string = [&]() {
        [[maybe_unused]] std::string s = value.get_optional<std::string>("111");
        return true;
    };

    // Null
    BOOST_TEST(value);
    BOOST_TEST(!value.null());

    // To int
    BOOST_TEST(value.get<int>() == 13);
    BOOST_TEST(value.get_optional<int>(42) == 13);

    // To double
    BOOST_TEST(value.get<double>() == 13.0);
    BOOST_TEST(value.get_optional<double>(42.0) == 13.0);

    // To string_view
    BOOST_TEST(value.get() == "13");
    BOOST_TEST(value.get_optional("42") == "13");
    BOOST_TEST(value.get_optional(sv44) == "13");

    // To string
    BOOST_TEST(value.get<std::string>() == "13");
    BOOST_TEST(value.get_optional<std::string>("42") == "13");
    BOOST_TEST(value.get_optional<std::string>(std::string("42")) == "13");
    BOOST_TEST(value.get_optional(s43) == "13");

    // Assign to string
    BOOST_TEST(assign_to_string());
}

BOOST_AUTO_TEST_CASE(sql_value_string_test)
{
    dbm::kind::sql_value value("hello", strlen("hello"));

    // Null
    BOOST_TEST(value);
    BOOST_TEST(!value.null());

    // To int
    BOOST_REQUIRE_THROW(value.get<int>(), std::exception);
    BOOST_TEST(value.get_optional<int>(42) == 42);

    // To double
    BOOST_REQUIRE_THROW(value.get<double>(), std::exception);
    BOOST_TEST(value.get_optional<double>(42.0) == 42.0);

    // String
    BOOST_TEST(value.get() == "hello");
    BOOST_TEST(value.get<std::string>() == "hello");
    BOOST_TEST(value.get<std::string>() == "hello");
    BOOST_TEST(value.get_optional("xxx") == "hello");
    BOOST_TEST(value.get_optional(std::string("xxx")) == "hello");
    BOOST_TEST(value.get_optional<std::string>("xxx") == "hello");
}

BOOST_AUTO_TEST_CASE(sql_value_null_test)
{
    dbm::kind::sql_value value(nullptr, 0);

    // Null
    BOOST_TEST(!value);
    BOOST_TEST(value.null());

    // To int
    BOOST_REQUIRE_THROW(value.get<int>(), std::exception);
    BOOST_TEST(value.get_optional<int>(42) == 42);

    // To double
    BOOST_REQUIRE_THROW(value.get<double>(), std::exception);
    BOOST_TEST(value.get_optional<double>(42.0) == 42.0);

    // Stirng
    BOOST_TEST(value.get_optional("xxx") == "xxx");
    BOOST_TEST(value.get_optional(std::string("xxx")) == "xxx");
    BOOST_TEST(value.get_optional<std::string>("xxx") == "xxx");
}

BOOST_AUTO_TEST_CASE(sql_value_copy_move_test)
{
    dbm::kind::sql_value value("hello", strlen("hello"));

    auto vcopy(value);

    BOOST_TEST(!value.null());
    BOOST_TEST(!vcopy.null());

    BOOST_TEST(value.get() == "hello");
    BOOST_TEST(vcopy.get() == "hello");

    auto vmove(std::move(vcopy));

    BOOST_TEST(!value.null());
    BOOST_TEST(vcopy.null());
    BOOST_TEST(!vmove.null());

    BOOST_TEST(value.get() == "hello");
    BOOST_TEST(vmove.get() == "hello");
}

BOOST_AUTO_TEST_CASE(sql_rows_copy)
{
    static constexpr size_t NCOLS = 3;
    static constexpr size_t NROWS = 3;
    std::array<const char*, NCOLS> data[NROWS] = {
        {"1", "ivo", "13"},
        { "2", "tarzan", "42"},
        { "3", "jane", nullptr }
    };

    dbm::sql_rows rows;

    {
        dbm::kind::sql_fields fields;
        fields.push_back("id");
        fields.push_back("name");
        fields.push_back("score");
        rows.set_field_names(std::move(fields));
    }

    for (const auto& R : data) {
        auto& row = rows.emplace_back();
        row.set_fields(&rows.field_names(), &rows.field_map());
        for (auto const& it : R) {
            row.emplace_back(it, it ? strlen(it) : 0);
        }
    }

    auto validate_rows = [&](dbm::sql_rows& r)
    {
        BOOST_TEST(r.at(0).field_names() == &r.field_names());
        BOOST_TEST(r.at(0).field_map() == &r.field_map());
        BOOST_TEST(r.at(1).field_names() == &r.field_names());
        BOOST_TEST(r.at(1).field_map() == &r.field_map());
        BOOST_TEST(r.at(2).field_names() == &r.field_names());
        BOOST_TEST(r.at(2).field_map() == &r.field_map());

        BOOST_TEST(r.at(0).at("id").get() == "1");
        BOOST_TEST(r.at(0).at("name").get() == "ivo");
        BOOST_TEST(r.at(0).at("score").get() == "13");
        BOOST_TEST(r.at(1).at("id").get() == "2");
        BOOST_TEST(r.at(1).at("name").get() == "tarzan");
        BOOST_TEST(r.at(1).at("score").get() == "42");
        BOOST_TEST(r.at(2).at("id").get() == "3");
        BOOST_TEST(r.at(2).at("name").get() == "jane");
        BOOST_TEST(r.at(2).at("score").null());
    };

    BOOST_TEST(rows.at(0).field_names() == &rows.field_names());

    dbm::sql_rows rows_copy = rows;
    BOOST_TEST(rows_copy.at(0).field_names() != &rows.field_names());
    BOOST_TEST(rows_copy.at(0).field_map() != &rows.field_map());
    validate_rows(rows_copy);

    dbm::sql_rows rows_copy_assign;
    rows_copy_assign = rows;
    BOOST_TEST(rows_copy_assign.at(0).field_names() != &rows.field_names());
    BOOST_TEST(rows_copy_assign.at(0).field_map() != &rows.field_map());
    validate_rows(rows_copy_assign);

    dbm::sql_rows rows_move = std::move(rows);
    BOOST_TEST(rows.empty());
    BOOST_TEST(rows_move.size() == NROWS);
    BOOST_TEST(rows_move.at(0).field_names() != &rows.field_names());
    BOOST_TEST(rows_move.at(0).field_map() != &rows.field_map());
    validate_rows(rows_move);

    dbm::sql_rows  rows_move_assign;
    rows_move_assign = std::move(rows_copy_assign);
    BOOST_TEST(rows_copy_assign.empty());
    BOOST_TEST(rows_move_assign.size() == NROWS);
    BOOST_TEST(rows_move_assign.at(0).field_names() != &rows.field_names());
    BOOST_TEST(rows_move_assign.at(0).field_map() != &rows.field_map());
    validate_rows(rows_move_assign);

    // dump test
    dbm::sql_rows_dump dump(rows_move_assign);
    dbm::sql_rows rows_from_dump;
    dump.restore(rows_from_dump);
    validate_rows(rows_from_dump);
}
