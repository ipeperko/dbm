#include <dbm/dbm.hpp>
#include "dbm/nlohmann_json_serializer.hpp"
#include "dbm/xml_serializer.hpp"

#include <boost/test/unit_test.hpp>

using namespace boost::unit_test;

BOOST_AUTO_TEST_SUITE(TstSerializer)

BOOST_AUTO_TEST_CASE(model_test_xml_serialization2, *tolerance(0.00001))
{
    time_t time = 1658560000;

    dbm::model model("tbl", {
                                { dbm::local<int>(13), dbm::key("id") },
                                { dbm::local<std::string>("honda"), dbm::key("name") },
                                { dbm::local(220.0), dbm::key("speed"), dbm::tag("tag_speed") },
                                { dbm::local(1500.0), dbm::key("weight"), dbm::taggable(false) },
                                { dbm::local<int>(), dbm::key("not_defined") },
                                { dbm::local(true), dbm::key("bool") },
                                { dbm::local(int16_t(1)), dbm::key("int16_t") },
                                { dbm::local(int32_t(1)), dbm::key("int32_t") },
                                { dbm::local(int64_t(1)), dbm::key("int64_t") },
                                { dbm::local(uint16_t(1)), dbm::key("uint16_t") },
                                { dbm::local(uint32_t(1)), dbm::key("uint32_t") },
                                { dbm::local(uint64_t(1)), dbm::key("uint64_t") },
                                { dbm::local<std::string>(nullptr, dbm::init_null::null), dbm::key("nillable") },
                                { dbm::timestamp(time), dbm::key("time") }
                            });

    dbm::xml::node xml1("xml");
    dbm::xml::node xml2("xml");
    dbm::xml::serializer ser2(xml2);

    model >> dbm::xml::serializer(xml1); // model rvalue reference
    //std::cout << xml1.to_string() << "\n";

    model >> ser2; // model lvalue reference
    //std::cout << xml2.to_string() << "\n";

    BOOST_TEST(xml1.to_string() == R"(<?xml version="1.0" encoding="utf-8"?><xml><id>13</id><name>honda</name><tag_speed>220</tag_speed><bool>1</bool><int16_t>1</int16_t><int32_t>1</int32_t><int64_t>1</int64_t><uint16_t>1</uint16_t><uint32_t>1</uint32_t><uint64_t>1</uint64_t><nillable xis:nil="true"></nillable><time>1658560000</time></xml>)");
    BOOST_TEST(xml2.to_string() == R"(<?xml version="1.0" encoding="utf-8"?><xml><id>13</id><name>honda</name><tag_speed>220</tag_speed><bool>1</bool><int16_t>1</int16_t><int32_t>1</int32_t><int64_t>1</int64_t><uint16_t>1</uint16_t><uint32_t>1</uint32_t><uint64_t>1</uint64_t><nillable xis:nil="true"></nillable><time>1658560000</time></xml>)");

    auto set_data = [](dbm::xml::node& x) {
        x.set("id", 14);
        x.set("name", "fiat");
        x.set("tag_speed", 200);
        x.set("weight", 1700);
        x.set("bool", false);
        x.set("int32_t", 2);
        x.set("int16_t", 3);
        x.set("int64_t", 4);
        x.set("uint32_t", 5);
        x.set("uint16_t", 6);
        x.set("uint64_t", 7);
        x.find("nillable")->attributes().clear();
        x.set("nillable", "not null");
        x.set("time", 1658566000);
    };

    // set new data
    dbm::xml::node xml_old = xml1;
    set_data(xml1);
    set_data(xml2);

    // set with model rvalue reference
    dbm::xml::serializer(xml1) >> model;
    BOOST_TEST(model.at("id").value<int>() == 14);
    BOOST_TEST(model.at("name").value<std::string>() == "fiat");
    BOOST_TEST(model.at("speed").value<double>() == 200);
    BOOST_TEST(model.at("weight").value<double>() == 1500.0); // should keep the original value as the item is not taggable
    BOOST_TEST(model.at("bool").value<bool>() == false);
    BOOST_TEST(model.at("int32_t").value<int32_t>() == 2);
    BOOST_TEST(model.at("int16_t").value<int16_t>() == 3);
    BOOST_TEST(model.at("int64_t").value<int64_t>() == 4);
    BOOST_TEST(model.at("uint32_t").value<uint32_t>() == 5);
    BOOST_TEST(model.at("uint16_t").value<uint16_t>() == 6);
    BOOST_TEST(model.at("uint64_t").value<uint64_t>() == 7);
    BOOST_TEST(model.at("nillable").is_null() == false);
    BOOST_TEST(model.at("nillable").value<std::string>() == "not null");
    BOOST_TEST(model.at("time").value<dbm::timestamp2u_converter>().get() == 1658566000);

    // reset model
    dbm::xml::serializer(xml_old) >> model;
    BOOST_TEST(model.at("id").value<int>() == 13);
    BOOST_TEST(model.at("name").value<std::string>() == "honda");
    BOOST_TEST(model.at("speed").value<double>() == 220);
    BOOST_TEST(model.at("weight").value<double>() == 1500.0); // should keep the original value as the item is not taggable
    BOOST_TEST(model.at("bool").value<bool>() == true);
    BOOST_TEST(model.at("int32_t").value<int32_t>() == 1);
    BOOST_TEST(model.at("int16_t").value<int16_t>() == 1);
    BOOST_TEST(model.at("int64_t").value<int64_t>() == 1);
    BOOST_TEST(model.at("uint32_t").value<uint32_t>() == 1);
    BOOST_TEST(model.at("uint16_t").value<uint16_t>() == 1);
    BOOST_TEST(model.at("uint64_t").value<uint64_t>() == 1);
    BOOST_TEST(model.at("nillable").is_null() == true);
    BOOST_TEST(model.at("time").value<dbm::timestamp2u_converter>().get() == 1658560000);

    // set with model lvalue reference
    ser2 >> model;
    BOOST_TEST(model.at("id").value<int>() == 14);
    BOOST_TEST(model.at("name").value<std::string>() == "fiat");
    BOOST_TEST(model.at("speed").value<double>() == 200);
    BOOST_TEST(model.at("weight").value<double>() == 1500.0); // should keep the original value as the item is not taggable
    BOOST_TEST(model.at("bool").value<bool>() == false);
    BOOST_TEST(model.at("int32_t").value<int32_t>() == 2);
    BOOST_TEST(model.at("int16_t").value<int16_t>() == 3);
    BOOST_TEST(model.at("int64_t").value<int64_t>() == 4);
    BOOST_TEST(model.at("uint32_t").value<uint32_t>() == 5);
    BOOST_TEST(model.at("uint16_t").value<uint16_t>() == 6);
    BOOST_TEST(model.at("uint64_t").value<uint64_t>() == 7);
    BOOST_TEST(model.at("nillable").is_null() == false);
    BOOST_TEST(model.at("nillable").value<std::string>() == "not null");
    BOOST_TEST(model.at("time").value<dbm::timestamp2u_converter>().get() == 1658566000);
}

BOOST_AUTO_TEST_CASE(model_test_json_serialization2, *tolerance(0.00001))
{
    time_t time = 1658560000;

    dbm::model model("tbl", {
                                { dbm::local<int>(13), dbm::key("id") },
                                { dbm::local<std::string>("honda"), dbm::key("name") },
                                { dbm::local(220.0), dbm::key("speed"), dbm::tag("tag_speed") },
                                { dbm::local(1500.0), dbm::key("weight"), dbm::taggable(false) },
                                { dbm::local<int>(), dbm::key("not_defined") },
                                { dbm::local(true), dbm::key("bool") },
                                { dbm::local(int16_t(1)), dbm::key("int16_t") },
                                { dbm::local(int32_t(1)), dbm::key("int32_t") },
                                { dbm::local(int64_t(1)), dbm::key("int64_t") },
                                { dbm::local(uint16_t(1)), dbm::key("uint16_t") },
                                { dbm::local(uint32_t(1)), dbm::key("uint32_t") },
                                { dbm::local(uint64_t(1)), dbm::key("uint64_t") },
                                { dbm::local<std::string>(nullptr, dbm::init_null::null), dbm::key("nillable") },
                                { dbm::timestamp(time), dbm::key("time") }
                            });

    nlohmann::json j1;
    nlohmann::json j2;
    nlohmann::serializer ser2(j2);

    model >> nlohmann::serializer(j1); // model rvalue reference
    std::cout << j1.dump() << "\n";

    model >> ser2; // model lvalue reference
    std::cout << j2.dump() << "\n";

    BOOST_TEST(j1.dump() == R"({"bool":true,"id":13,"int16_t":1,"int32_t":1,"int64_t":1,"name":"honda","nillable":null,"tag_speed":220.0,"time":1658560000,"uint16_t":1,"uint32_t":1,"uint64_t":1})");
    BOOST_TEST(j2.dump() == R"({"bool":true,"id":13,"int16_t":1,"int32_t":1,"int64_t":1,"name":"honda","nillable":null,"tag_speed":220.0,"time":1658560000,"uint16_t":1,"uint32_t":1,"uint64_t":1})");

    auto set_data = [](nlohmann::json& j) {
        j["id"] = 14;
        j["name"] = "fiat";
        j["tag_speed"] = 200;
        j["weight"] = 1700;
        j["bool"] = false;
        j["int32_t"] = 2;
        j["int16_t"] = 3;
        j["int64_t"] = 4;
        j["uint32_t"] = 5;
        j["uint16_t"] = 6;
        j["uint64_t"] = 7;
        j["nillable"] = "not null";
        j["time"] = 1658566000;
    };

    // set new data
    nlohmann::json j_old = j1;
    set_data(j1);
    set_data(j2);

    // set with model rvalue reference
    nlohmann::serializer(j1) >> model;
    BOOST_TEST(model.at("id").value<int>() == 14);
    BOOST_TEST(model.at("name").value<std::string>() == "fiat");
    BOOST_TEST(model.at("speed").value<double>() == 200);
    BOOST_TEST(model.at("weight").value<double>() == 1500.0); // should keep the original value as the item is not taggable
    BOOST_TEST(model.at("bool").value<bool>() == false);
    BOOST_TEST(model.at("int32_t").value<int32_t>() == 2);
    BOOST_TEST(model.at("int16_t").value<int16_t>() == 3);
    BOOST_TEST(model.at("int64_t").value<int64_t>() == 4);
    BOOST_TEST(model.at("uint32_t").value<uint32_t>() == 5);
    BOOST_TEST(model.at("uint16_t").value<uint16_t>() == 6);
    BOOST_TEST(model.at("uint64_t").value<uint64_t>() == 7);
    BOOST_TEST(model.at("nillable").is_null() == false);
    BOOST_TEST(model.at("nillable").value<std::string>() == "not null");
    BOOST_TEST(model.at("time").value<dbm::timestamp2u_converter>().get() == 1658566000);

    // reset model
    nlohmann::serializer(j_old) >> model;
    BOOST_TEST(model.at("id").value<int>() == 13);
    BOOST_TEST(model.at("name").value<std::string>() == "honda");
    BOOST_TEST(model.at("speed").value<double>() == 220);
    BOOST_TEST(model.at("weight").value<double>() == 1500.0); // should keep the original value as the item is not taggable
    BOOST_TEST(model.at("bool").value<bool>() == true);
    BOOST_TEST(model.at("int32_t").value<int32_t>() == 1);
    BOOST_TEST(model.at("int16_t").value<int16_t>() == 1);
    BOOST_TEST(model.at("int64_t").value<int64_t>() == 1);
    BOOST_TEST(model.at("uint32_t").value<uint32_t>() == 1);
    BOOST_TEST(model.at("uint16_t").value<uint16_t>() == 1);
    BOOST_TEST(model.at("uint64_t").value<uint64_t>() == 1);
    BOOST_TEST(model.at("nillable").is_null() == true);
    BOOST_TEST(model.at("time").value<dbm::timestamp2u_converter>().get() == 1658560000);

    // set with model lvalue reference
    ser2 >> model;
    BOOST_TEST(model.at("id").value<int>() == 14);
    BOOST_TEST(model.at("name").value<std::string>() == "fiat");
    BOOST_TEST(model.at("speed").value<double>() == 200);
    BOOST_TEST(model.at("weight").value<double>() == 1500.0); // should keep the original value as the item is not taggable
    BOOST_TEST(model.at("bool").value<bool>() == false);
    BOOST_TEST(model.at("int32_t").value<int32_t>() == 2);
    BOOST_TEST(model.at("int16_t").value<int16_t>() == 3);
    BOOST_TEST(model.at("int64_t").value<int64_t>() == 4);
    BOOST_TEST(model.at("uint32_t").value<uint32_t>() == 5);
    BOOST_TEST(model.at("uint16_t").value<uint16_t>() == 6);
    BOOST_TEST(model.at("uint64_t").value<uint64_t>() == 7);
    BOOST_TEST(model.at("nillable").is_null() == false);
    BOOST_TEST(model.at("nillable").value<std::string>() == "not null");
    BOOST_TEST(model.at("time").value<dbm::timestamp2u_converter>().get() == 1658566000);
}

BOOST_AUTO_TEST_CASE(nlohmann_json_test)
{
    const nlohmann::json j {
        { "id", 1 },
        { "name", "rambo" },
        { "score", 2.34 },
        { "armed", true }
    };

    int id;
    BOOST_REQUIRE_NO_THROW(id = j.at("id").get<int>());
    BOOST_TEST(id == 1);
    BOOST_REQUIRE_NO_THROW(id = j.at("id").get<double>());
    BOOST_TEST(id == 1);
    BOOST_REQUIRE_THROW(j.at("id").get<std::string>(), std::exception);

    std::string name;
    BOOST_REQUIRE_NO_THROW(name = j.at("name").get<std::string>());
    BOOST_TEST(name == "rambo");
    BOOST_REQUIRE_THROW(j.at("name").get<int>(), std::exception);

    double score;
    BOOST_REQUIRE_NO_THROW(score = j.at("score").get<double>());
    BOOST_TEST(score == 2.34);
    BOOST_REQUIRE_NO_THROW(score = j.at("score").get<int>());
    BOOST_TEST(score == 2);
    BOOST_REQUIRE_THROW(j.at("score").get<std::string>(), std::exception);

    bool armed = false;
    BOOST_REQUIRE_NO_THROW(armed = j.at("armed").get<bool>());
    BOOST_TEST(armed == true);
    armed = false;
    BOOST_REQUIRE_NO_THROW(armed = j.at("armed").get<int>());
    BOOST_TEST(armed == true);
}

BOOST_AUTO_TEST_CASE(const_json_serializer, *tolerance(0.00001))
{
    using parse_result = dbm::kind::parse_result;

    const nlohmann::json j {
        { "id", 1 },
        { "name", "rambo" },
        { "score", 2.34 },
        { "armed", true },
        { "id_s", "2" },
        { "score_s", "3.45" },
    };

    nlohmann::serializer ser2(j);

    {
        auto [res, val] = ser2.deserialize<int>("not_exists");
        BOOST_TEST((res == parse_result::undefined));
    }

    {
        auto [res, val] = nlohmann::serializer(j).deserialize<int>("id");
        BOOST_TEST((res == parse_result::ok));
        BOOST_TEST(val.value() == 1);
    }

    {
        auto [res, val] = nlohmann::serializer(j).deserialize<int>("id_s"); // should fail because id_s is of type string
        BOOST_TEST((res == parse_result::error));
        BOOST_TEST(val.has_value() == false);
    }

    {
        auto [res, val] = nlohmann::serializer(j).deserialize<double>("score");
        BOOST_TEST((res == parse_result::ok));
        BOOST_TEST(val.value() == 2.34);
    }

    {
        auto [res, val] = nlohmann::serializer(j).deserialize<int>("score_s"); // should fail because score_s is of type string
        BOOST_TEST((res == parse_result::error));
        BOOST_TEST(val.has_value() == false);
    }
}

BOOST_AUTO_TEST_SUITE_END()