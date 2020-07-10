#include "dbm.hpp"
#ifdef DBM_MYSQL
#include "drivers/mysql/mysql_session.hpp"
#endif
#ifdef DBM_SQLITE3
#include "drivers/sqlite/sqlite_session.hpp"
#endif
#include "nlohmann_json_serializer.hpp"
#include "xml_serializer.hpp"

#include <boost/test/unit_test.hpp>
#include <iomanip>

using namespace boost::unit_test;

static constexpr const char* dbm_test_db_name = "dbm_test";
static constexpr const char* dbm_test_db_file_name = "dbm_test.sqlite3";
static constexpr const char* dbm_test_table_name = "test_table";

#ifdef DBM_MYSQL
static std::string mysql_host;
static std::string mysql_username;
static std::string mysql_password;
static int mysql_port = 3306;

BOOST_AUTO_TEST_CASE(db_config)
{
    mysql_host = "127.0.0.1";

    int argc = framework::master_test_suite().argc;

    for (int i = 0; i < argc - 1; i++) {
        std::string arg = framework::master_test_suite().argv[i];

        if (arg == "--mysql-host") {
            mysql_host = framework::master_test_suite().argv[i+1];
        }
        else if (arg == "--mysql-username" || arg == "--db-username") {
            mysql_username = framework::master_test_suite().argv[i+1];
        }
        else if (arg == "--mysql-password" || arg == "--db-password") {
            mysql_password = framework::master_test_suite().argv[i+1];
        }
    }

    if (mysql_host.empty()) {
        BOOST_FAIL("mysql host name not defined");
    }
    if (mysql_username.empty()) {
        BOOST_FAIL("mysql username not defined");
    }
    if (mysql_password.empty()) {
        BOOST_FAIL("mysql password not defined");
    }
}
#endif

BOOST_AUTO_TEST_CASE(model_test_set_table_name)
{
    dbm::model m;
    m.set_table_name("my_table");
    BOOST_TEST(m.table_name() == "my_table");
}

BOOST_AUTO_TEST_CASE(model_test_add_remove_items)
{
    dbm::model m;
    m.emplace_back(dbm::key("first"), dbm::tag("first"));
    m.emplace_back(dbm::key("second"));
    m.emplace_back(dbm::key("third"));
    BOOST_TEST(m.items().at(0).tag().get() == "first");
    BOOST_TEST(m.at("first").tag().get() == "first");
    BOOST_TEST(m.at("first").tag().get() == "first");
    BOOST_TEST(m.at("second").key().get() == "second");
    BOOST_TEST(m.at("third").key().get() == "third");
    bool res;
    BOOST_TEST((res = m.find("third") != m.end()));
    BOOST_TEST((res = m.find("not_there") == m.end()));
    BOOST_TEST(m.find("third")->key().get() == "third");

    auto it = m.find("third");
    if (it != m.end()) {
        std::cout << it->key() << " " << it->value<std::string>() << " " << it->is_null() << "\n";
    }

    m.erase("second");
    BOOST_TEST(m.items().size() == 2);
    BOOST_TEST(m.items().at(1).key().get() == "third");
}

BOOST_AUTO_TEST_CASE(model_test_replace_container)
{
    dbm::model m;
    int val = 1;
    m.emplace_back(dbm::key("test"), dbm::local<int>(2));
    m.at("test").set(dbm::binding(val));
}

BOOST_AUTO_TEST_CASE(model_test_serialization)
{
    dbm::model model("tbl", {
        { dbm::local<int>(13), dbm::key("id") },
        { dbm::local<std::string>("honda"), dbm::key("name") },
        { dbm::local(220), dbm::key("speed"), dbm::tag("tag_speed") },
        { dbm::local(1500), dbm::key("weight"), dbm::taggable(false) },
        { dbm::local<int>(), dbm::key("not_defined") },
                      });

    BOOST_TEST(model.at("not_defined").is_null());
    BOOST_TEST(!model.at("not_defined").is_defined());

    dbm::xml::node xml("xml");
    dbm::xml::serializer serializer(xml);
    model >> serializer;

    auto xml_includes = [&](std::string_view k) {
        return xml.find(k) != xml.end();
    };

    BOOST_TEST(xml_includes("id"));
    BOOST_TEST(xml.get<int>("id") == 13);
    BOOST_TEST(xml_includes("name"));
    BOOST_TEST(xml.get<std::string>("name") == "honda");
    BOOST_TEST(!xml_includes("not_defined"));

    model.at("not_defined").set(dbm::required(true));
    BOOST_REQUIRE_THROW(model.serialize(serializer), std::exception);
}

int getActiveProfileId(int controller_id) {

    int active_profile_id;
    dbm::model model("table",
                     {
                         { dbm::key("profile"), dbm::local(active_profile_id) },
                         { dbm::key("profile2"), dbm::binding(active_profile_id) }
                     });
    return active_profile_id;
}

BOOST_AUTO_TEST_CASE(webserver_table_active_profile)
{
    getActiveProfileId(1);
}



namespace {
struct DataFields
{
    int id {0};
    std::string name;
    int int_not_null {0};
};
} // namespace

static DataFields data_fields;

template<typename SessionType>
dbm::model get_test_model()
{
    std::string autoincr_str;
    std::string ts;

#ifdef DBM_MYSQL
    if constexpr (std::is_same_v<SessionType, dbm::mysql_session>) {
        autoincr_str = " AUTO_INCREMENT";
        ts = " NOT NULL DEFAULT CURRENT_TIMESTAMP";
    }
#endif
#ifdef DBM_SQLITE3
    if constexpr (std::is_same_v<SessionType, dbm::sqlite_session>) {
        //        autoincr_str = " AUTOINCREMENT";
        ts = " NOT NULL DEFAULT CURRENT_TIMESTAMP";
    }
#endif

    return dbm::model {
        dbm_test_table_name,
        {
            { dbm::binding(data_fields.id),
              dbm::key("id"),
              dbm::tag("tag_id"),
              dbm::primary(true),
              dbm::dbtype("INTEGER NOT NULL" + autoincr_str) },
            { dbm::binding(data_fields.name),
              dbm::key("text_not_null"),
              dbm::tag("tag_text_not_null"),
              dbm::dbtype("TEXT NOT NULL") },
            { dbm::local<std::string>(),
              dbm::key("text_with_null"),
              dbm::tag("tag_text_with_null"),
              dbm::required(true),
              dbm::dbtype("TEXT DEFAULT NULL") },
            { dbm::binding(data_fields.int_not_null),
              dbm::key("int_not_null"),
              dbm::tag("tag_int_not_null"),
              dbm::dbtype("INTEGER NOT NULL") },
            { dbm::local<int>(),
              dbm::key("int_with_null"),
              dbm::tag("tag_int_with_null"),
              dbm::dbtype("INTEGER DEFAULT NULL") },
            { dbm::local<bool>(),
              dbm::key("tiny_int_not_null"),
              dbm::tag("tag_tiny_int_not_null"),
              dbm::dbtype("TINYINT NOT NULL") },
            { dbm::local<bool>(),
              dbm::key("tiny_int_with_null"),
              dbm::tag("tag_tiny_int_with_null"),
              dbm::dbtype("TINYINT DEFAULT NULL") },
            { dbm::local<std::string>(),
              dbm::key("timestamp"),
              dbm::tag("timestamp_tag"),
              dbm::dbtype("TIMESTAMP" + ts) },
        }
    };
}

template<typename SessionType>
void test_mysql()
{
    SessionType session;
    const std::string db_name = dbm_test_db_name;
    const std::string tbl_name = dbm_test_table_name;

#ifdef DBM_MYSQL
    if constexpr (std::is_same_v<SessionType, dbm::mysql_session>) {
        BOOST_TEST_MESSAGE("MySQL test\n");

        SessionType tmp_session;
        tmp_session.init(mysql_host, mysql_username, mysql_password, mysql_port, "");
        tmp_session.open();
        tmp_session.create_database(db_name, true);

        session.init(mysql_host, mysql_username, mysql_password, mysql_port, db_name);
    }
#endif
#ifdef DBM_SQLITE3
    if constexpr (std::is_same_v<SessionType, dbm::sqlite_session>) {
        BOOST_TEST_MESSAGE("SQLite test\n");
        session.set_db_name(dbm_test_db_file_name);
    }
#endif

    session.open();

    dbm::model model = get_test_model<SessionType>();
    model.drop_table(session);
    model.create_table(session);

    session.query("INSERT INTO " + tbl_name +
                  R"( (text_not_null, text_with_null, int_not_null, int_with_null, tiny_int_not_null, tiny_int_with_null)
VALUES
("ivo", "qqq", "1", "2", "0", "1"),
("ipo", NULL, 13, NULL, 1, NULL))");

    auto rows = session.select("SELECT * FROM " + tbl_name);

    std::ostringstream os_tbl;
    size_t w = 0;

    for (const auto& it : rows.field_names()) {
        if (it.length() > w) {
            w = it.length();
        }
    }

    w += 1;

    for (const auto& it : rows.field_names()) {
        os_tbl << std::setw(w) << std::left << it;
    }
    os_tbl << "\n";
    os_tbl << std::right << std::setfill('-') << std::setw(w * rows.field_names().size() + 1) << "\n"
           << std::setfill(' ');

    for (const auto& row : rows) {
        for (const auto& val : row) {
            os_tbl << std::setw(w) << std::left << val;
        }
        os_tbl << "\n";
    }

    BOOST_TEST_MESSAGE(os_tbl.str());

    os_tbl = std::ostringstream();
    os_tbl << std::right << std::setfill('-') << std::setw(w * rows.field_names().size() + 1) << "\n"
           << std::setfill(' ');

    for (const auto& row : rows) {
        for (const auto& key : rows.field_names()) {
            os_tbl << std::left << std::setw(w) << row.at(key);
        }
        os_tbl << "\n";
    }
    os_tbl << "\n";
    BOOST_TEST_MESSAGE(os_tbl.str());

    session.close();
}

BOOST_AUTO_TEST_CASE(sql_test)
{
#ifdef DBM_MYSQL
    test_mysql<dbm::mysql_session>();
#endif

#ifdef DBM_SQLITE3
    test_mysql<dbm::sqlite_session>();
#endif
}

template<typename SessionType>
void test_model()
{
    using namespace dbm;

    data_fields = {};
    data_fields.id = 1;

    SessionType session;
    const std::string db_name = dbm_test_db_name;
    const std::string tbl_name = dbm_test_table_name;

#ifdef DBM_MYSQL
    if constexpr (std::is_same_v<SessionType, dbm::mysql_session>) {
        BOOST_TEST_MESSAGE("MySQL model test\n");
        session.init(mysql_host, mysql_username, mysql_password, mysql_port, db_name);
    }
#endif
#ifdef DBM_SQLITE3
    if constexpr (std::is_same_v<SessionType, dbm::sqlite_session>) {
        BOOST_TEST_MESSAGE("SQLite model test\n");
        session.set_db_name(dbm_test_db_file_name);
    }
#endif

    session.open();

    dbm::xml::node xml("xml");
    nlohmann::json json;

    dbm::xml::serializer xml_serializer(xml);
    nlohmann::serializer json_serializer(json);

    model model = get_test_model<SessionType>();

    // Add model item - not present in database
    //model.items().emplace_back(dbm::key("model_only")); // doesn't exist in database

    // Add database item - not present in model
    session.query("ALTER TABLE " + model.table_name() + " ADD COLUMN db_only INTEGER DEFAULT NULL");
    session.query("ALTER TABLE " + model.table_name() + " ADD COLUMN db_only2 INTEGER DEFAULT NULL");

    // serialize
    session >> model >> xml_serializer;

    BOOST_TEST_MESSAGE(xml.to_string(4) << "\n");

    {
        // Test model copy
        dbm::xml::node xml2("xml");
        dbm::model(model) >> dbm::xml::serializer(xml2);
        BOOST_TEST(xml2.to_string() == xml.to_string());
    }

    // change value in xml object
    xml_serializer.set("tag_text_not_null", "iii");
    xml_serializer >> model;
    BOOST_TEST(model.at(dbm::key("text_not_null")).value<std::string>() == "iii");
    model >> session;

    // delete record
    model.delete_record(session);
    {
        auto s2(session);
        s2.open();
        auto rows = s2.select("SELECT * FROM " + tbl_name + " WHERE id=" + std::to_string(data_fields.id));
        BOOST_TEST(rows.empty());
    }

    // reset value in xml object and write record
    xml_serializer.set("tag_text_not_null", "ivo");
    xml_serializer >> model;
    BOOST_TEST(model.at(dbm::key("text_not_null")).value<std::string>() == "ivo");
    model >> session;

    // nlohmann json test
    model >> json_serializer;
    BOOST_TEST_MESSAGE(json.dump(4) << "\n");

    // change value in json and write record
    json["tag_text_not_null"] = "jjjj";
    json_serializer >> model;
    BOOST_TEST(model.at(dbm::key("text_not_null")).value<std::string>() == "jjjj");
    model >> session;

    {
        // Test model copy
        nlohmann::json j2;
        dbm::model(model) >> nlohmann::serializer(j2);
        BOOST_TEST(j2.dump() == json.dump());
    }

    // delete record
    model.delete_record(session);
    {
        auto s2(session);
        s2.open();
        auto rows = s2.select("SELECT * FROM " + tbl_name + " WHERE id=" + std::to_string(data_fields.id));
        BOOST_TEST(rows.empty());
    }

    // reset value in json and write record
    json["tag_text_not_null"] = "ivo";
    json_serializer >> model;
    BOOST_TEST(model.at(dbm::key("text_not_null")).value<std::string>() == "ivo");
    model >> session;

    {
        auto s2(session);
        s2.open();
        auto rows = s2.select("SELECT * FROM " + tbl_name + " WHERE id=" + std::to_string(data_fields.id));
        BOOST_TEST(rows.size() == 1);
    }

    // write NULL in text field
    std::string text_with_null = json["tag_text_with_null"];
    json["tag_text_with_null"] = nullptr;
    json_serializer >> model;
    BOOST_TEST(model.at("text_with_null").is_null());
    model >> session;

    model.at(dbm::key("text_with_null")).set_value(std::string("xxx"));
    BOOST_TEST(model.at(dbm::key("text_with_null")).to_string() == "xxx");
    session >> model;
    BOOST_TEST(model.at(dbm::key("text_with_null")).is_null());

    json["tag_text_with_null"] = text_with_null;
    json_serializer >> model;
    BOOST_TEST(model.at(dbm::key("text_with_null")).to_string() == text_with_null);
    model >> session;
}

BOOST_AUTO_TEST_CASE(model_test)
{
#ifdef DBM_MYSQL
    test_model<dbm::mysql_session>();
#endif

#ifdef DBM_SQLITE3
    test_model<dbm::sqlite_session>();
#endif
}

BOOST_AUTO_TEST_CASE(const_json_serializer)
{
    using parse_result = dbm::deserializer::parse_result;

    const nlohmann::json j {
        { "id", 1 },
        { "name", "rambo"},
        { "score", 2.34 },
        { "armed", true },
        { "id_s", "2" },
        { "score_s", "3.45" },
    };


    nlohmann::deserializer ser2(j);
    int a;

    parse_result res;
    res = ser2.deserialize("not_exists", a);
    BOOST_TEST(res == parse_result::undefined);

    int id = -1;
    BOOST_TEST(nlohmann::deserializer(j).deserialize("id", id) == parse_result::ok);
    BOOST_TEST(id == 1);
    id = -1;
    BOOST_TEST(nlohmann::deserializer(j).deserialize("id_s", id) == parse_result::error);

    double score = 0;
    BOOST_TEST(nlohmann::deserializer(j).deserialize("score", score) == parse_result::ok);
    BOOST_TEST(score == 2.34);
    score = 0;
    BOOST_TEST(nlohmann::deserializer(j).deserialize("score_s", score) == parse_result::error);
}


BOOST_AUTO_TEST_CASE(model_find_item)
{
    dbm::model m("",
                      {
                          { dbm::local(1), dbm::key("controller_id"), dbm::primary(true), dbm::taggable(false) },
                          { dbm::local<time_t>(), dbm::key("time"), dbm::primary(true) },
                          { dbm::local<int>(), dbm::key("element_id"), dbm::primary(true) },
                          { dbm::local<double>(), dbm::key("value") },
                      });

    bool test = std::end(m.items()) == m.end();
    BOOST_TEST(test);
    test = m.end() == m.items().end();
    BOOST_TEST(test);

    auto it = m.find("time");
    test = it != m.end();
    BOOST_TEST(test);

    m.items().erase(it);

    it = m.find("time");
    test = it == m.end();
    BOOST_TEST(test);
}

BOOST_AUTO_TEST_CASE(nlohmann_json_test)
{
    const nlohmann::json j {
        { "id", 1 },
        { "name", "rambo"},
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
