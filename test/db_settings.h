#ifndef DBM_DB_SETTINGS_H
#define DBM_DB_SETTINGS_H

#include <dbm/dbm.hpp>
#ifdef DBM_MYSQL
#include <dbm/drivers/mysql/mysql_session.hpp>
#endif
#ifdef DBM_SQLITE3
#include <dbm/drivers/sqlite/sqlite_session.hpp>
#endif

namespace dbm {
class mysql_session;
class sqlite_session;
}

class db_settings
{
public:
    db_settings(const db_settings&) = delete;
    db_settings(db_settings&&) = delete;
    db_settings& operator=(const db_settings&) = delete;
    db_settings& operator=(db_settings&&) = delete;

    static db_settings& instance();

    void init_mysql_session(dbm::mysql_session& session, std::string const& db_name = instance().test_db_name);
    void init_sqlite_session(dbm::sqlite_session& session);

    std::unique_ptr<dbm::mysql_session> get_mysql_session();
    std::unique_ptr<dbm::sqlite_session> get_sqlite_session();

    template<typename DBType>
    std::unique_ptr<DBType> get_session()
    {
        if constexpr (std::is_same_v<DBType, dbm::mysql_session>) {
            return get_mysql_session();
        }
        else if constexpr (std::is_same_v<DBType, dbm::sqlite_session>) {
            return get_sqlite_session();
        }
    }

    template<typename DBType>
    void init_session(DBType& s)
    {
        if constexpr (std::is_same_v<DBType, dbm::mysql_session>) {
            init_mysql_session(s);
        }
        else if constexpr (std::is_same_v<DBType, dbm::sqlite_session>) {
            init_sqlite_session(s);
        }
    }

    std::string mysql_host {"127.0.0.1"};
    std::string mysql_username;
    std::string mysql_password;
    int mysql_port {3306};

    const char* test_db_name = "dbm_test";
    const char* test_db_file_name = "dbm_test.sqlite3";
    const char* test_table_name = "test_table";

private:
    db_settings();
    void initialize();
};

#ifdef DBM_MYSQL
struct MakeMySqlSession
{
    std::shared_ptr<dbm::mysql_session> operator()();
};

using MySqlPool = dbm::pool<dbm::mysql_session, MakeMySqlSession>;
#endif

#ifdef DBM_SQLITE3
struct MakeSQLiteSession
{
    std::shared_ptr<dbm::sqlite_session> operator()();
};

using SQLitePool = dbm::pool<dbm::sqlite_session, MakeSQLiteSession>;
#endif

#endif //DBM_DB_SETTINGS_H
