#ifndef DBM_MYSQL_SESSION_HPP
#define DBM_MYSQL_SESSION_HPP

#include <dbm/session.hpp>

namespace dbm {

class DBM_EXPORT mysql_session : public session
{
public:
    mysql_session();
    mysql_session(const mysql_session& oth);
    mysql_session(mysql_session&& oth) noexcept;
    mysql_session& operator=(const mysql_session& oth);
    mysql_session& operator=(mysql_session&& oth) noexcept;
    ~mysql_session() override;

    void init(const std::string& host_name, const std::string& user, const std::string& pass, unsigned int port, const std::string& db_name, unsigned int flags = 0);

    /* Set database name doesn't work if already connected! */
    void set_database_name(const std::string& name);

    void open() override;
    void close() override;

    void query(const std::string& statement) override;

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

    std::string opt_host_name;       /* server host (e.g.: localhost) */
    std::string opt_user_name;       /* username */
    std::string opt_password;        /* password */
    unsigned int opt_port_num{3306}; /* port number */
    char* opt_socket_name{nullptr};  /* socket name (NULL = use built-in value) */
    std::string opt_db_name;         /* database name (default=none) */
    unsigned int opt_flags{0};       /* connection flags (none) */

    void* conn__{nullptr};    /* connection handler pointer */
    void* res_set__{nullptr}; /* mysql result set */
};

}// namespace dbm

#endif//DBM_MYSQL_SESSION_HPP
