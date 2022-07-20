#ifndef DBM_SQLITE_SESSION_HPP
#define DBM_SQLITE_SESSION_HPP

#include <dbm/session.hpp>

struct sqlite3;

namespace dbm {

class DBM_EXPORT sqlite_session : public session<sqlite_session>
{
    friend class session;
public:
    sqlite_session() = default;
    sqlite_session(const sqlite_session& oth);
    sqlite_session(sqlite_session&& oth) = delete;
    sqlite_session& operator=(const sqlite_session& oth);
    sqlite_session& operator=(sqlite_session&& oth) = delete;
    ~sqlite_session();

    void connect(std::string_view db_file_name);

private:
    void close_impl();
    bool is_connected_impl() const { return db3_ != nullptr; }

    void query_impl(std::string_view statement);
    void init_prepared_statement_impl(kind::prepared_statement& stmt);
    void query_impl(kind::prepared_statement& stmt);
    std::vector<std::vector<container_ptr>> select_impl(kind::prepared_statement& stmt);

    std::string write_model_query_impl(const model& m) const;
    std::string read_model_query_impl(const model& m, const std::string& extra_condition) const;
    std::string delete_model_query_impl(const model& m) const;
    std::string create_table_query_impl(const model& m, bool if_not_exists) const;
    std::string drop_table_query_impl(const model& m, bool if_exists) const;

    void transaction_begin_impl();
    void transaction_commit_impl();
    void transaction_rollback_impl();

    kind::sql_rows select_rows_impl(std::string_view statement);
    void free_table();
    void destroy_prepared_stmt_handles();

//    std::string db_name_;       // database file name
    sqlite3* db3_{nullptr};      // database handle
    char** azResult{nullptr};   // results of the query
};

}// namespace dbm

#endif//DBM_SQLITE_SESSION_HPP
