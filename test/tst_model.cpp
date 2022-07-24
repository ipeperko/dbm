#include <dbm/dbm.hpp>
#ifdef DBM_MYSQL
#include <dbm/drivers/mysql/mysql_session.hpp>
#endif
#ifdef DBM_SQLITE3
#include <dbm/drivers/sqlite/sqlite_session.hpp>
#endif
#include "dbm/nlohmann_json_serializer.hpp"
#include "dbm/xml_serializer.hpp"

#include <boost/test/unit_test.hpp>
#include <iomanip>

#include "db_settings.h"

using namespace boost::unit_test;

BOOST_AUTO_TEST_SUITE(TstModel)

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

BOOST_AUTO_TEST_CASE(model_item_timestamp)
{
    time_t storage = 42;
    auto item = dbm::model_item(dbm::timestamp(storage));

    BOOST_TEST(item.value<dbm::timestamp2u_converter>().has_reference());
    BOOST_TEST(item.to_string() == "42");
    BOOST_TEST(item.value<dbm::timestamp2u_converter>().get() == 42);

    storage = 123;
    BOOST_TEST(item.to_string() == "123");
    BOOST_TEST(item.value<dbm::timestamp2u_converter>().get() == 123);

    item.from_string("666");
    BOOST_TEST(item.to_string() == "666");
    BOOST_TEST(item.value<dbm::timestamp2u_converter>().get() == 666);
}

BOOST_AUTO_TEST_CASE(model_item_swap)
{
    double vald = 3.14;

    dbm::model_item m1(dbm::key("key_1"),
                       dbm::tag("tag_1"),
                       dbm::custom_data_type("INTEGER"),
                       dbm::local<int>(42),
                       dbm::taggable(true),
                       dbm::direction::bidirectional,
                       dbm::create(true),
                       dbm::primary(true),
                       dbm::auto_increment(true),
                       dbm::not_null(true),
                       dbm::defaultc(std::nullopt),
                       dbm::valquotes(true)
                       );

    dbm::model_item m2(dbm::key("key_2"),
                       dbm::tag("tag_2"),
                       dbm::custom_data_type(""),
                       dbm::binding(vald),
                       dbm::taggable(false),
                       dbm::direction::disabled,
                       dbm::create(false),
                       dbm::primary(false),
                       dbm::auto_increment(false),
                       dbm::not_null(false),
                       dbm::defaultc(nullptr),
                       dbm::valquotes(false)
                       );

    std::swap(m1, m2);

    BOOST_TEST(m1.key().get() == "key_2");
    BOOST_TEST(m2.key().get() == "key_1");
    BOOST_TEST(m1.tag().get() == "tag_2");
    BOOST_TEST(m2.tag().get() == "tag_1");
    BOOST_TEST(m1.custom_data_type().get() == "");
    BOOST_TEST(m2.custom_data_type().get() == "INTEGER");
    BOOST_TEST(m1.value<double>() == 3.14);
    BOOST_TEST(m2.value<int>() == 42);
    BOOST_TEST(m1.conf().taggable() == false);
    BOOST_TEST(m2.conf().taggable());
    BOOST_TEST(m1.conf().writable() == false);
    BOOST_TEST(m2.conf().writable());
    BOOST_TEST(m1.conf().readable() == false);
    BOOST_TEST(m2.conf().readable());
    BOOST_TEST(m1.conf().creatable() == false);
    BOOST_TEST(m2.conf().creatable());
    BOOST_TEST(m1.conf().primary() == false);
    BOOST_TEST(m2.conf().primary());
    BOOST_TEST(m1.conf().auto_increment() == false);
    BOOST_TEST(m2.conf().auto_increment());
    BOOST_TEST(m1.conf().not_null() == false);
    BOOST_TEST(m2.conf().not_null());
    BOOST_TEST(m1.conf().valquotes() == false);
    BOOST_TEST(m2.conf().valquotes());
}

BOOST_AUTO_TEST_CASE(model_swap)
{
    double vald = 3.14;

    dbm::model model1("table_1", {
        {
            dbm::key("m1_key1"),
            dbm::tag("m1_tag1"),
            dbm::custom_data_type("INTEGER"),
            dbm::local<int>(42),
            dbm::taggable(true),
            dbm::direction::bidirectional,
            dbm::create(true),
            dbm::primary(true),
            dbm::auto_increment(true),
            dbm::not_null(true),
            dbm::valquotes(true)
        },
        {
            dbm::key("m1_key2"),
            dbm::tag("m1_tag2"),
            dbm::custom_data_type("REAL"),
            dbm::local<double>(666.666),
            dbm::taggable(true),
            dbm::direction::bidirectional,
            dbm::create(true),
            dbm::primary(true),
            dbm::auto_increment(true),
            dbm::not_null(true),
            dbm::valquotes(true)
        }
    });

    dbm::model model2("table_2", {
        {
            dbm::key("m2_key1"),
            dbm::tag("m2_tag1"),
            dbm::custom_data_type(""),
            dbm::binding<double>(vald),
            dbm::taggable(false),
            dbm::direction::disabled,
            dbm::create(false),
            dbm::primary(false),
            dbm::auto_increment(false),
            dbm::not_null(false),
            dbm::valquotes(false)
        }
    });

    std::swap(model1, model2);

    BOOST_TEST(model1.table_name() == "table_2");
    BOOST_TEST(model2.table_name() == "table_1");
    BOOST_TEST(model1.items().size() == 1);
    BOOST_TEST(model2.items().size() == 2);

    auto const& m1 = model1.items().at(0);
    auto const& m2 = model2.items().at(0);
    auto const& m3 = model2.items().at(1);

    BOOST_TEST(m1.key().get() == "m2_key1");
    BOOST_TEST(m2.key().get() == "m1_key1");
    BOOST_TEST(m3.key().get() == "m1_key2");
    BOOST_TEST(m1.tag().get() == "m2_tag1");
    BOOST_TEST(m2.tag().get() == "m1_tag1");
    BOOST_TEST(m3.tag().get() == "m1_tag2");
    BOOST_TEST(m1.custom_data_type().get() == "");
    BOOST_TEST(m2.custom_data_type().get() == "INTEGER");
    BOOST_TEST(m3.custom_data_type().get() == "REAL");
    BOOST_TEST(m1.value<double>() == 3.14);
    BOOST_TEST(m2.value<int>() == 42);
    BOOST_TEST(m3.value<double>() == 666.666);
    BOOST_TEST(m1.conf().taggable() == false);
    BOOST_TEST(m2.conf().taggable());
    BOOST_TEST(m3.conf().taggable());
    BOOST_TEST(m1.conf().writable() == false);
    BOOST_TEST(m2.conf().writable());
    BOOST_TEST(m3.conf().writable());
    BOOST_TEST(m1.conf().readable() == false);
    BOOST_TEST(m2.conf().readable());
    BOOST_TEST(m3.conf().readable());
    BOOST_TEST(m1.conf().creatable() == false);
    BOOST_TEST(m2.conf().creatable());
    BOOST_TEST(m3.conf().creatable());
    BOOST_TEST(m1.conf().primary() == false);
    BOOST_TEST(m2.conf().primary());
    BOOST_TEST(m3.conf().primary());
    BOOST_TEST(m1.conf().auto_increment() == false);
    BOOST_TEST(m2.conf().auto_increment());
    BOOST_TEST(m3.conf().auto_increment());
    BOOST_TEST(m1.conf().not_null() == false);
    BOOST_TEST(m2.conf().not_null());
    BOOST_TEST(m3.conf().not_null());
    BOOST_TEST(m1.conf().valquotes() == false);
    BOOST_TEST(m2.conf().valquotes());
    BOOST_TEST(m3.conf().valquotes());
}

int getActiveProfileId(int controller_id)
{
    int active_profile_id;
    dbm::model model("table",
                     { { dbm::key("profile"), dbm::local(active_profile_id) },
                       { dbm::key("profile2"), dbm::binding(active_profile_id) } });
    return active_profile_id;
}

BOOST_AUTO_TEST_CASE(webserver_table_active_profile)
{
    getActiveProfileId(1);
}

namespace {
struct DataFields
{
    int id { 0 };
    std::string name;
    int int_not_null { 0 };
};
} // namespace

static DataFields data_fields;

template<typename SessionType>
dbm::model get_test_model()
{
    auto& db_sett = db_settings::instance();
    std::string autoincr_str;
    std::string ts;

#ifdef DBM_MYSQL
    if constexpr (std::is_same_v<SessionType, dbm::mysql_session>) {
        autoincr_str = " AUTO_INCREMENT";
    }
#endif
#ifdef DBM_SQLITE3
    if constexpr (std::is_same_v<SessionType, dbm::sqlite_session>) {
        //        autoincr_str = " AUTOINCREMENT";
    }
#endif

    dbm::model m {
        db_sett.test_table_name,
        { { dbm::binding(data_fields.id),
            dbm::key("id"),
            dbm::tag("tag_id"),
            dbm::primary(true),
            //dbm::dbtype("INTEGER NOT NULL" + autoincr_str),
            dbm::not_null(true),
            dbm::auto_increment(true)
          },
          { dbm::binding(data_fields.name),
            dbm::key("text_not_null"),
            dbm::tag("tag_text_not_null"),
            //dbm::dbtype("TEXT NOT NULL"),
            dbm::not_null(true)
          },
          { dbm::local<std::string>(),
            dbm::key("text_with_null"),
            dbm::tag("tag_text_with_null"),
            dbm::required(true),
            //dbm::dbtype("TEXT DEFAULT NULL")
          },
          { dbm::binding(data_fields.int_not_null),
            dbm::key("int_not_null"),
            dbm::tag("tag_int_not_null"),
            //dbm::dbtype("INTEGER NOT NULL"),
            dbm::not_null(true),
            dbm::defaultc(0)
          },
          { dbm::local<int>(),
            dbm::key("int_with_null"),
            dbm::tag("tag_int_with_null"),
            //dbm::dbtype("INTEGER DEFAULT NULL")
          },
          { dbm::local<bool>(),
            dbm::key("tiny_int_not_null"),
            dbm::tag("tag_tiny_int_not_null"),
            dbm::custom_data_type("TINYINT NOT NULL DEFAULT 1"),
            //dbm::not_null(true)
          },
          { dbm::local<bool>(),
            dbm::key("tiny_int_with_null"),
            dbm::tag("tag_tiny_int_with_null"),
            //dbm::dbtype("TINYINT DEFAULT NULL"),
          },
          { dbm::local<std::string>(),
            dbm::key("timestamp"),
            dbm::tag("timestamp_tag"),
            dbm::custom_data_type("TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP"),
            //dbm::valquotes(false)
          } }
    };

#ifdef DBM_MYSQL
    if constexpr (std::is_same_v<SessionType, dbm::mysql_session>) {
        m.emplace_back(dbm::local<int64_t>(),
                       dbm::key("UNIX_TIMESTAMP(timestamp)"),
                       dbm::tag("unixtime_tag"),
                       dbm::direction::read_only,
                       dbm::create(false));
    }
#endif
#ifdef DBM_SQLITE3
    if constexpr (std::is_same_v<SessionType, dbm::sqlite_session>) {
        m.emplace_back(dbm::local<int64_t>(),
                       dbm::key("strftime('%s',timestamp)"),
                       dbm::tag("unixtime_tag"),
                       dbm::direction::read_only,
                       dbm::create(false));
    }
#endif

    return m;
}

template<typename SessionType>
void test_table()
{
    SessionType session;
    auto& db_sett = db_settings::instance();
    const std::string db_name = db_sett.test_db_name;
    const std::string tbl_name = db_sett.test_table_name;

#ifdef DBM_MYSQL
    if constexpr (std::is_same_v<SessionType, dbm::mysql_session>) {
        BOOST_TEST_MESSAGE("MySQL test\n");

        SessionType tmp_session;
        db_sett.init_mysql_session(tmp_session, "");
        tmp_session.create_database(db_name, true);
        db_sett.init_mysql_session(session, db_name);
    }
#endif
#ifdef DBM_SQLITE3
    if constexpr (std::is_same_v<SessionType, dbm::sqlite_session>) {
        BOOST_TEST_MESSAGE("SQLite test\n");
        session.connect(db_sett.test_db_file_name);
    }
#endif

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
    test_table<dbm::mysql_session>();
#endif

#ifdef DBM_SQLITE3
    test_table<dbm::sqlite_session>();
#endif
}

template<typename SessionType>
void test_model()
{
    using namespace dbm;

    data_fields = {};
    data_fields.id = 1;

    SessionType session;
    auto& db_sett = db_settings::instance();
    db_sett.template init_session<SessionType>(session);

    const std::string tbl_name = db_sett.test_table_name;

    dbm::xml::node xml("xml");
    nlohmann::json json;

    dbm::xml::serializer2 xml_serializer(xml);
    nlohmann::serializer2 json_serializer(json);

    model model = get_test_model<SessionType>();

    // Add database item - not present in model
    session.query("ALTER TABLE " + model.table_name() + " ADD COLUMN db_only INTEGER DEFAULT NULL");
    session.query("ALTER TABLE " + model.table_name() + " ADD COLUMN db_only2 INTEGER DEFAULT NULL");

    // serialize
    session >> model >> xml_serializer;

    BOOST_TEST_MESSAGE(xml.to_string(4) << "\n");

    {
        // Test model copy
        dbm::xml::node xml2("xml");
        dbm::model(model) >> dbm::xml::serializer2(xml2);
        BOOST_TEST(xml2.to_string() == xml.to_string());
    }

    // change value in xml object
    xml_serializer.serialize("tag_text_not_null", "iii");
    xml_serializer >> model;
    BOOST_TEST(model.at(dbm::key("text_not_null")).value<std::string>() == "iii");
    model >> session;

    // delete record
    model.delete_record(session);
    {
        SessionType s2;
        db_sett.template init_session<SessionType>(s2);
        dbm::statement q;
        q << "SELECT * FROM " << tbl_name << " WHERE id=" << data_fields.id;
        auto rows = s2.select(q);
        BOOST_TEST(rows.empty());
    }

    // reset value in xml object and write record
    xml_serializer.serialize("tag_text_not_null", "ivo");
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
        dbm::model(model) >> nlohmann::serializer2(j2);
        BOOST_TEST(j2.dump() == json.dump());
    }

    // delete record
    model.delete_record(session);
    {
        SessionType s2;
        db_sett.template init_session<SessionType>(s2);
        dbm::statement q;
        q << "SELECT * FROM " << tbl_name << " WHERE id=" << data_fields.id;
        auto rows = s2.select(q);
        BOOST_TEST(rows.empty());
    }

    // reset value in json and write record
    json["tag_text_not_null"] = "ivo";
    json_serializer >> model;
    BOOST_TEST(model.at(dbm::key("text_not_null")).value<std::string>() == "ivo");
    model >> session;

    {
        SessionType s2;
        db_sett.template init_session<SessionType>(s2);
        dbm::statement q;
        q << "SELECT * FROM " << tbl_name << " WHERE id=" << data_fields.id;
        auto rows = s2.select(q);
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

BOOST_AUTO_TEST_CASE(model_find_item)
{
    dbm::model m("",
                 {
                     { dbm::local(1), dbm::key("controller_id"), dbm::primary(true), dbm::taggable(false) },
                     { dbm::local<int64_t>(), dbm::key("time"), dbm::primary(true) },
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

template<typename SessionType>
void test_model_with_timestamp()
{
    dbm::model m {
        "test_timestamp",
        { { dbm::local<int>(1),
            dbm::key("id"),
            dbm::primary(true),
            dbm::not_null(true),
            dbm::auto_increment(true)
            },
          }
    };

    //dbm::model_item& item_timestamp_string = //
    m.emplace_back(dbm::local<std::string>(),
            dbm::key("timestamp"),
            dbm::tag("timestamp_string"),
            dbm::custom_data_type("TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP"),
            dbm::direction::read_only
            );

#ifdef DBM_MYSQL
    if constexpr (std::is_same_v<SessionType, dbm::mysql_session>) {
        m.set_table_options("ENGINE=MEMORY");

        m.emplace_back(dbm::local<int64_t>(),
            dbm::key("UNIX_TIMESTAMP(timestamp)"),
            dbm::tag("unixtime"),
            dbm::direction::read_only,
            dbm::create(false));
    }
#endif

#ifdef DBM_SQLITE3
    if constexpr (std::is_same_v<SessionType, dbm::sqlite_session>) {
        m.emplace_back(dbm::local<int64_t>(),
            dbm::key("strftime('%s',timestamp)"),
            dbm::tag("unixtime"),
            dbm::direction::read_only,
            dbm::create(false));
    }
#endif

    m.emplace_back(dbm::local<std::string>(),
        dbm::key("timestamp"),
        dbm::tag("timestamp_write_only"),
        dbm::direction::write_only,
        dbm::valquotes(false), // quotes are disabled as we are writing functions
        dbm::create(false));

    SessionType session;
    auto& db_sett = db_settings::instance();
    db_sett.template init_session<SessionType>(session);

    // Create table
    m.drop_table(session);
    m.create_table(session);

    // Insert one record - timestamp will be current time
    m >> session;

    // Read record to get timestamp
    BOOST_TEST_MESSAGE(session.read_model_query(m, ""));
    session >> m;

    auto& item_timestamp_string = m.items().at(1);
    auto& item_read_unixtime = m.items().at(2);
    auto& item_write_unixtime = m.items().back();

    BOOST_TEST_MESSAGE("Read timestamp " + item_read_unixtime.to_string() + " " + item_timestamp_string.to_string());

    // Set timestamp functions
#ifdef DBM_MYSQL
    if constexpr (std::is_same_v<SessionType, dbm::mysql_session>) {
        item_write_unixtime.set_value("FROM_UNIXTIME(12345678)");
    }
#endif
#ifdef DBM_SQLITE3
    if constexpr (std::is_same_v<SessionType, dbm::sqlite_session>) {
        item_write_unixtime.set_value("datetime(12345678, 'unixepoch')");
    }
#endif
    m >> session;
    item_read_unixtime.set_value(0);

    // Raad value and validate
    session >> m;
    BOOST_TEST(item_read_unixtime.to_string() == "12345678");
}

BOOST_AUTO_TEST_CASE(model_with_timestamp)
{
#ifdef DBM_MYSQL
    test_model_with_timestamp<dbm::mysql_session>();
#endif
#ifdef DBM_SQLITE3
    test_model_with_timestamp<dbm::sqlite_session>();
#endif
}

template<typename SessionType>
void test_model_timestamp2u()
{
    time_t time = 12345678;

    dbm::model m {
        "test_timestamp2u",
        { { dbm::local<int>(1),
            dbm::key("id"),
            dbm::primary(true),
            dbm::not_null(true),
            dbm::auto_increment(true)
          },
          { dbm::timestamp(time),
            dbm::key("time"),
            dbm::not_null(true),
            dbm::defaultc("CURRENT_TIMESTAMP")
          } }
    };

    SessionType session;
    auto& db_sett = db_settings::instance();
    db_sett.template init_session<SessionType>(session);

    // Create table
    m.drop_table(session);
    m.create_table(session);

    // Insert one record - timestamp will be '1970-05-23 21:21:18'
    m >> session;

    // Reset time
    time = 0;
    BOOST_TEST(m.find("time")->to_string() == "0");

    // Read record to get timestamp
    BOOST_TEST_MESSAGE(session.read_model_query(m, ""));
    session >> m;
    BOOST_TEST(m.find("time")->to_string() == "12345678");
}

BOOST_AUTO_TEST_CASE(model_timestamp2u)
{
#ifdef DBM_MYSQL
    test_model_timestamp2u<dbm::mysql_session>();
#endif
#ifdef DBM_SQLITE3
    test_model_with_timestamp<dbm::sqlite_session>();
#endif
}

BOOST_AUTO_TEST_SUITE_END()