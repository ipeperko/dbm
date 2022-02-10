#ifndef DBM_DB_SETTINGS_H
#define DBM_DB_SETTINGS_H

#include <dbm/dbm.hpp>

namespace dbm {
class session;
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
    void init_mysql_session(dbm::session& session, std::string const& db_name = instance().test_db_name);

    void init_sqlite_session(dbm::sqlite_session& session);
    void init_sqlite_session(dbm::session& session);

    std::unique_ptr<dbm::session> get_mysql_session();
    std::unique_ptr<dbm::session> get_sqlite_session();

    template<typename DBType>
    void init_session(dbm::session& s)
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
class MakeMySqlSession
{
public:
    std::shared_ptr<dbm::mysql_session> operator()();
};

class MySqlPool : public dbm::pool<MakeMySqlSession>
{
public:
    MySqlPool()
        : dbm::pool<MakeMySqlSession>(MakeMySqlSession())
    {
    }
};
#endif

#ifdef DBM_SQLITE3
class MakeSQLiteSession
{
public:
    std::shared_ptr<dbm::sqlite_session> operator()();
};

class SQLitePool : public dbm::pool<MakeSQLiteSession>
{
public:
    SQLitePool()
        : dbm::pool<MakeSQLiteSession>(MakeSQLiteSession())
    {
    }
};
#endif

#endif //DBM_DB_SETTINGS_H
