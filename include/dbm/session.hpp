#ifndef DBM_SESSION_HPP
#define DBM_SESSION_HPP

#include <dbm/dbm_common.hpp>
#include <dbm/sql_types.hpp>
#include <dbm/prepared_statement.hpp>

#include <unordered_map>

namespace dbm {

class model;

/*!
 * Database session interface class
 */
class DBM_EXPORT session
{
public:
    class transaction;

    session() = default;
    session(const session& oth);
    session(session&& oth) noexcept;
    session& operator=(const session& oth) ;
    session& operator=(session&& oth) noexcept;
    virtual ~session() = default;

    virtual std::unique_ptr<session> clone() const = 0;

    virtual void open() = 0;
    virtual void close() = 0;

    virtual bool is_connected() const = 0;

    virtual void query(const std::string& statement); // TODO: string_view
    virtual void query(const detail::statement& q);
    kind::sql_rows select(const std::string& statement); // TODO: string_view
    kind::sql_rows select(const std::vector<std::string>& what, const std::string& table, const std::string& criteria="");
    kind::sql_rows select(const detail::statement& q);

    virtual void init_prepared_statement(dbm::kind::prepared_statement& stmt) {}
    virtual void query(kind::prepared_statement& stmt) {}
    virtual std::vector<std::vector<container_ptr>> select_prepared_statement(kind::prepared_statement& stmt) { return {}; }

    virtual std::string write_model_query(const model& m) const = 0;
    virtual std::string read_model_query(const model& m, const std::string& extra_condition="") const = 0;
    virtual std::string delete_model_query(const model& m) const = 0;
    virtual std::string create_table_query(const model& m, bool if_not_exists) const = 0;
    virtual std::string drop_table_query(const model& m, bool if_exists) const = 0;

    virtual void transaction_begin() = 0;
    virtual void transaction_commit() = 0;
    virtual void transaction_rollback() = 0;

    void create_database(const std::string& db_name, bool if_not_exists);
    void drop_database(const std::string& db_name, bool if_exists);
    void create_table(const model& m, bool if_not_exists);
    void drop_table(const model& m, bool if_exists);

    std::string const& last_statement() const noexcept { return last_statement_; }
    std::string last_statement_info() const;

    model& operator>>(model& m);
    model&& operator>>(model&& m);

    auto& prepared_statement_handles() { return prepared_stm_handle_; }
    auto const& prepared_statement_handles() const { return prepared_stm_handle_; }

protected:
    virtual kind::sql_rows select_rows(const std::string& statement) { return {}; }

    std::string last_statement_;
    std::unordered_map<std::string, void*> prepared_stm_handle_;
};

/*!
 * Database transaction class
 */
class DBM_EXPORT session::transaction
{
public:
    explicit transaction(session& db);

    ~transaction();

    void commit();

    void rollback();

private:
    void perform(bool do_commit);

    dbm::session& db_;
    bool executed_ {false};
};

}// namespace dbm

#endif//DBM_SESSION_HPP
