#ifndef DBM_DB_SETTINGS_H
#define DBM_DB_SETTINGS_H

#include <string>
#include <memory>

namespace dbm {
class session;
class mysql_session;
class postgres_session;
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

    void init_mysql_session(dbm::mysql_session& session, std::string const& db_name);
    void init_postgres_session(dbm::postgres_session& session, std::string const& db_name);

    std::unique_ptr<dbm::session> get_mysql_session();
    std::unique_ptr<dbm::session> get_postgres_session();
    std::unique_ptr<dbm::session> get_sqlite_session();

    std::string mysql_host {"127.0.0.1"};
    std::string mysql_username;
    std::string mysql_password;
    int mysql_port {3306};

    std::string postgres_host {"127.0.0.1"};
    std::string postgres_username;
    std::string postgres_password;
    int postgres_port {3306};

    const char* test_db_name = "dbm_test";
    const char* test_db_file_name = "dbm_test.sqlite3";
    const char* test_table_name = "test_table";

private:
    db_settings();
    void initialize();
};

#endif //DBM_DB_SETTINGS_H
