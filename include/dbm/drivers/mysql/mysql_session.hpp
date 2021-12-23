#ifndef DBM_MYSQL_SESSION_HPP
#define DBM_MYSQL_SESSION_HPP

#include <dbm/session.hpp>

namespace dbm {

class DBM_EXPORT mysql_session : public session
{
public:
    mysql_session() = default;
    mysql_session(const mysql_session& oth);
    mysql_session(mysql_session&& oth) noexcept;
    mysql_session& operator=(const mysql_session& oth);
    mysql_session& operator=(mysql_session&& oth) noexcept;
    ~mysql_session() override;

    void connect(
        std::optional<std::string_view> host,
        std::optional<std::string_view> user,
        std::optional<std::string_view> passwd,
        std::optional<std::string_view> db,
        unsigned int port = 0,
        std::optional<std::string_view> unix_socket = std::nullopt,
        unsigned long client_flag = 0);

    void close() override;

    bool is_connected() const override { return conn_ != nullptr; }

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
    std::string drop_table_query(const model& m, bool if_exist) const override;

    void transaction_begin() override;
    void transaction_commit() override;
    void transaction_rollback() override;

private:
    kind::sql_rows select_rows(const std::string& statement) override;
    void free_result_set();
    std::string last_mysql_error() const;

    void* conn_{nullptr};    /* connection handler pointer */
    void* res_set_{nullptr}; /* mysql result set */
};

}// namespace dbm

#endif//DBM_MYSQL_SESSION_HPP
