#ifndef DBM_SQLITE_SESSION_HPP
#define DBM_SQLITE_SESSION_HPP

#include "../../session.hpp"

struct sqlite3;

namespace dbm {

class sqlite_session : public session
{
public:
    sqlite_session();
    explicit sqlite_session(std::string_view db_name);
    sqlite_session(const sqlite_session& oth);
    sqlite_session(sqlite_session&& oth) = delete;
    sqlite_session& operator=(const sqlite_session& oth);
    sqlite_session& operator=(sqlite_session&& oth) = delete;
    ~sqlite_session() override;

    void set_db_name(std::string_view file_name);
    std::string_view db_name() const;

    void open() override;
    void close() override;

    void query(const std::string& statement) override;
    kind::sql_rows select(const std::string& statement) override;

    std::string write_model_query(const model& m) const override;
    std::string read_model_query(const model& m, const std::string& extra_condition) const override;
    std::string delete_model_query(const model& m) const override;
    std::string create_table_query(const model& m, bool if_not_exists) const override;
    std::string drop_table_query(const model& m, bool if_exists) const override;

    void transaction_begin() override { throw_exception<std::domain_error>("TODO sqlite transaction"); }
    void transaction_commit() override { throw_exception<std::domain_error>("TODO sqlite transaction"); }
    void transaction_rollback() override { throw_exception<std::domain_error>("TODO sqlite transaction"); }

private:
    void free_table();

    std::string db_name_;// database file name
    sqlite3* db3{nullptr};
    char** azResult{nullptr};
};

}// namespace dbm

#endif//DBM_SQLITE_SESSION_HPP
