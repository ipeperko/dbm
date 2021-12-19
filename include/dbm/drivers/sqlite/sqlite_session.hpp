#ifndef DBM_SQLITE_SESSION_HPP
#define DBM_SQLITE_SESSION_HPP

#include <dbm/session.hpp>

struct sqlite3;

namespace dbm {

class DBM_EXPORT sqlite_session : public session
{
public:
    sqlite_session();
    explicit sqlite_session(std::string_view db_name);
    sqlite_session(const sqlite_session& oth);
    sqlite_session(sqlite_session&& oth) = delete;
    sqlite_session& operator=(const sqlite_session& oth);
    sqlite_session& operator=(sqlite_session&& oth) = delete;
    ~sqlite_session() override;

    std::unique_ptr<session> clone() const override;

    void set_db_name(std::string_view file_name);
    std::string_view db_name() const;

    void open() override;
    void close() override;

    bool is_connected() const override { return db3_ != nullptr; }

    void* native_handle() const override { return db3_; }

    using session::query;
    void query(const std::string& statement) override;

    using session::select;

    void init_prepared_statement(kind::prepared_statement& stmt) override;
    void query(kind::prepared_statement& stmt) override;
    std::vector<std::vector<container_ptr>> select(kind::prepared_statement& stmt) override;

    std::string write_model_query(const model& m) const override;
    std::string read_model_query(const model& m, const std::string& extra_condition) const override;
    std::string delete_model_query(const model& m) const override;
    std::string create_table_query(const model& m, bool if_not_exists) const override;
    std::string drop_table_query(const model& m, bool if_exists) const override;

    void transaction_begin() override;
    void transaction_commit() override;
    void transaction_rollback() override;

private:
    kind::sql_rows select_rows(const std::string& statement) override;
    void free_table();
    void destroy_prepared_stmt_handles();

    std::string db_name_;       // database file name
    sqlite3* db3_{nullptr};      // database handle
    char** azResult{nullptr};   // results of the query
};

}// namespace dbm

#endif//DBM_SQLITE_SESSION_HPP
