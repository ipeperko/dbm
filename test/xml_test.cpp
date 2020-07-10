#include "xml.hpp"
#include <boost/test/unit_test.hpp>
#include <iomanip>

using namespace boost::unit_test;
namespace xml = dbm::xml;

BOOST_AUTO_TEST_CASE(xml_value_get_test)
{
    xml::node xml("xml");
    xml.add("one", 1);
    xml.add("string", "string value");

    BOOST_TEST(xml.get<int>("one") == 1);
    BOOST_TEST(xml.get<std::string>("one") == "1");
    BOOST_TEST(xml.get<std::string>("string") == "string value");
    BOOST_REQUIRE_THROW(xml.get<int>("string"), std::exception);
    BOOST_TEST(xml.get_optional<int>("string", 42) == 42);
}

BOOST_AUTO_TEST_CASE(xml_copy_test)
{
    xml::node xml("xml");
    xml.add("one", 1);
    xml.add("string", "string value");

    // copy ctor
    xml::node xml2(xml);
    BOOST_TEST(xml2.get<int>("one") == 1);
    BOOST_TEST(xml2.get<std::string>("one") == "1");
    BOOST_TEST(xml2.get<std::string>("string") == "string value");

    // copy assign
    auto& new_group = xml.add("new_group");
    new_group = xml2;
    BOOST_TEST(new_group.tag() == "xml");
    BOOST_TEST(new_group.get<int>("one") == 1);
    BOOST_TEST(new_group.get<std::string>("one") == "1");
    BOOST_TEST(new_group.get<std::string>("string") == "string value");
}

BOOST_AUTO_TEST_CASE(xml_move_test)
{
    xml::node xml("xml");
    xml.add("one", 1);
    xml.add("string", "string value");

    // move ctor
    xml::node xml2(std::move(xml));
    BOOST_TEST(xml.items().empty()); // source xml should be empty
    BOOST_TEST(xml.tag() == "xml"); // root item tag should not be moved
    BOOST_TEST(xml2.get<int>("one") == 1);
    BOOST_TEST(xml2.get<std::string>("one") == "1");
    BOOST_TEST(xml2.get<std::string>("string") == "string value");

    // move assign
    auto& new_group = xml.add("new_group");
    new_group = std::move(xml2);
    BOOST_TEST(xml2.items().empty()); // source xml should be empty
    BOOST_TEST(xml2.tag() == "xml"); // root item tag should not be moved
    BOOST_TEST(new_group.tag() == "xml");
    BOOST_TEST(new_group.get<int>("one") == 1);
    BOOST_TEST(new_group.get<std::string>("one") == "1");
    BOOST_TEST(new_group.get<std::string>("string") == "string value");
}

BOOST_AUTO_TEST_CASE(xml_test_parent_pointer)
{
    xml::node xml("xml");
    xml.add("one", 1);
    xml.add("string", "string value");

    xml.at(0).add("s", 1.1);

    BOOST_TEST(!xml.parent());
    BOOST_TEST(xml.at(0).parent() == &xml);
    BOOST_TEST(xml.at(0).at(0).parent() == &xml.at(0));
    BOOST_TEST(xml.at(1).parent() == &xml);

    // copy
    xml::node xml2 = xml;

    BOOST_TEST(!xml2.parent());
    BOOST_TEST(xml2.at(0).parent() == &xml2);
    BOOST_TEST(xml2.at(0).at(0).parent() == &xml2.at(0));
    BOOST_TEST(xml2.at(1).parent() == &xml2);
}