#ifdef DBM_MYSQL

#include "db_settings.h"
#include "common.h"
#include "dbm/dbm.hpp"
#include <dbm/drivers/mysql/mysql_session.hpp>

using namespace boost::unit_test;

namespace{

constexpr std::string_view table_name = "tst_mysql_procedure";
constexpr int n_inserts_pre_iter = 100;

std::unique_ptr<dbm::session> get_session()
{
    auto conn = std::make_unique<dbm::mysql_session>();
    db_settings::instance().init_mysql_session(*conn, db_settings::instance().test_db_name);
    conn->open();
    return conn;
}

auto get_model()
{
    return dbm::model(table_name.data(),
                 {
                     { dbm::key("id"), dbm::local<unsigned>(), dbm::primary(true), dbm::auto_increment(true), dbm::not_null(true) },
                     { dbm::key("value1"), dbm::local<std::string>() },
                     { dbm::key("value2"), dbm::local<std::string>() }
                 });
}

class Inserter
{
public:

    explicit Inserter(dbm::session& db)
        : db(db)
    {}

    dbm::session& db;
    int id {0};

    std::string value(int col) const
    {
        return std::to_string(col) + "_" + std::to_string(id);
    }

    void write_with_model()
    {
        auto m = get_model();
        auto n = id + n_inserts_pre_iter;

        for (; id < n; ++id) {
            m.at("value1").set_value(value(1));
            m.at("value2").set_value(value(2));
            m >> db;
        }
    }

    void write_with_prepared_stmt()
    {
        auto n = id + n_inserts_pre_iter;

        for (; id < n; ++id) {
            dbm::prepared_stmt stmt("INSERT INTO tst_mysql_procedure (value1, value2) VALUES(?, ?)",
                                    dbm::local<std::string>(value(1)),
                                    dbm::local<std::string>(value(2)));
            stmt >> db;
        }
    }
};

class Reader
{
public:

    explicit Reader(dbm::session& db)
        : db(db)
    {}

    dbm::session& db;
    int id {0};

    void read_with_select()
    {
        auto rows = db.select(dbm::statement() << "SELECT * FROM " << table_name <<
                               " WHERE id>" << id << " AND id<=" << id + n_inserts_pre_iter);
        BOOST_TEST(rows.size() == n_inserts_pre_iter);
        id += n_inserts_pre_iter;
    }
};

} // namespace

BOOST_AUTO_TEST_SUITE(TstMysqlProcedure)

BOOST_AUTO_TEST_CASE(table_init)
{
    auto db = get_session();
    auto m = get_model();
    m.drop_table(*db);
    m.create_table(*db);

    db->query("DROP PROCEDURE IF EXISTS tst_mysql_procedure_get_data");
    db->query(R"(CREATE PROCEDURE tst_mysql_procedure_get_data (p1 INT, p2 INT, p3 INT, p4 INT)
BEGIN
    SELECT * FROM tst_mysql_procedure WHERE id > p1 AND id <= p2
    UNION
    SELECT * FROM tst_mysql_procedure WHERE id > p2 AND id <= p3
    UNION
    SELECT * FROM tst_mysql_procedure WHERE id > p3 AND id <= p4;
END;)");

    Inserter inserter(*db);
    Reader reader(*db);

    inserter.write_with_model();
    reader.read_with_select();
    auto rows = db->select("CALL tst_mysql_procedure_get_data(0, 30, 70, 100)");
    BOOST_TEST(rows.size() == 100);

    // TODO: insert with prepared statement after CALL procedure fails
    // but simple insert works
    //inserter.write_with_prepared_stmt();
    inserter.write_with_model();
    reader.read_with_select();
    rows = db->select("CALL tst_mysql_procedure_get_data(100, 130, 170, 200)");
    BOOST_TEST(rows.size() == 100);
}

BOOST_AUTO_TEST_SUITE_END()

#endif