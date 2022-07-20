#ifndef DBM_MYSQL_SESSION_HPP
#define DBM_MYSQL_SESSION_HPP

#include <dbm/session.hpp>

namespace dbm {

class DBM_EXPORT mysql_session : public session<mysql_session>
{
    friend class session;
public:
    mysql_session() = default;
    mysql_session(const mysql_session& oth);
    mysql_session(mysql_session&& oth) noexcept;
    mysql_session& operator=(const mysql_session& oth);
    mysql_session& operator=(mysql_session&& oth) noexcept;
    ~mysql_session();

    void connect(
        std::optional<std::string_view> host,
        std::optional<std::string_view> user,
        std::optional<std::string_view> passwd,
        std::optional<std::string_view> db,
        unsigned int port = 0,
        std::optional<std::string_view> unix_socket = std::nullopt,
        unsigned long client_flag = 0);

private:
    void close_impl();
    bool is_connected_impl() const { return conn_ != nullptr; }

    void query_impl(std::string_view statement);
    void init_prepared_statement_impl(kind::prepared_statement& stmt);
    void query_impl(kind::prepared_statement& stmt);
    std::vector<std::vector<container_ptr>> select_impl(kind::prepared_statement& stmt);

    std::string write_model_query_impl(const model& m) const;
    std::string read_model_query_impl(const model& m, const std::string& extra_condition) const;
    std::string delete_model_query_impl(const model& m) const;
    std::string create_table_query_impl(const model& m, bool if_not_exists) const;
    std::string drop_table_query_impl(const model& m, bool if_exist) const;

    void transaction_begin_impl();
    void transaction_commit_impl();
    void transaction_rollback_impl();

    kind::sql_rows select_rows_impl(std::string_view statement);
    void free_result_set();
    std::string last_mysql_error() const;

    void* conn_{nullptr};    /* connection handler pointer */
    void* res_set_{nullptr}; /* mysql result set */
};

}// namespace dbm

#endif//DBM_MYSQL_SESSION_HPP
