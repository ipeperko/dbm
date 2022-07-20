#ifndef DBM_SESSION_HPP
#define DBM_SESSION_HPP

#include <dbm/dbm_common.hpp>
#include <dbm/sql_types.hpp>
#include <dbm/prepared_statement.hpp>

#include <unordered_map>

namespace dbm {

class model;

class DBM_EXPORT session_base
{
public:
};

/*!
 * Database session interface class
 */
template<typename Impl>
class DBM_EXPORT session : private session_base
{
public:

    class transaction;

    session() = default;
    session(const session& oth);
    session(session&& oth) noexcept;
    session& operator=(const session& oth);
    session& operator=(session&& oth) noexcept;

    auto& self() { return static_cast<Impl&>(*this); }
    auto const& self() const { return static_cast<Impl const&>(*this); }

    void close() { self().close_impl(); }
    bool is_connected() const { return self().is_connected_impl(); }

    void query(std::string_view statement) { self().query_impl(statement); }
    void query(const detail::statement& q) { self().query_impl(q.get()); }
    kind::sql_rows select(std::string_view statement) { return self().select_rows(statement); }
    kind::sql_rows select(const std::vector<std::string>& what, std::string_view table, std::string_view criteria="");
    kind::sql_rows select(const detail::statement& q) { return select(q.get()); }

    void init_prepared_statement(dbm::kind::prepared_statement& stmt) { self().init_prepared_statement_impl(stmt); }
    void remove_prepared_statement(std::string const& s);
    void query(kind::prepared_statement& stmt) { self().query_impl(stmt); }
    std::vector<std::vector<container_ptr>> select(kind::prepared_statement& stmt) { return self().select_impl(stmt); }

    std::string write_model_query(const model& m) const { return self().write_model_query_impl(m); }
    std::string read_model_query(const model& m, const std::string& extra_condition="") const { return self().read_model_query_impl(m, extra_condition); }
    std::string delete_model_query(const model& m) const { return self().delete_model_query_impl(m); }
    std::string create_table_query(const model& m, bool if_not_exists) const { return self().create_table_query_impl(m, if_not_exists); }
    std::string drop_table_query(const model& m, bool if_exists) const { return self().drop_table_query_impl(m, if_exists); }

    void transaction_begin() { self().transaction_begin_impl(); }
    void transaction_commit() { self().transaction_commit_impl(); }
    void transaction_rollback() { self().transaction_rollback_impl(); }

    void create_database(std::string_view db_name, bool if_not_exists);
    void drop_database(std::string_view db_name, bool if_exists);
    void create_table(const model& m, bool if_not_exists);
    void drop_table(const model& m, bool if_exists);

    std::string const& last_statement() const noexcept { return last_statement_; }
    std::string last_statement_info() const;

    model& operator>>(model& m);
    model&& operator>>(model&& m);

    auto& prepared_statement_handles() { return prepared_stm_handle_; }
    auto const& prepared_statement_handles() const { return prepared_stm_handle_; }

protected:
    kind::sql_rows select_rows(std::string_view statement) { return self().select_rows_impl(statement); }

    std::string last_statement_;
    std::unordered_map<std::string, void*> prepared_stm_handle_;
};

/*!
 * Database transaction class
 */
template<typename Impl>
class DBM_EXPORT session<Impl>::transaction
{
public:
    explicit transaction(session<Impl>& db);

    ~transaction();

    void commit();

    void rollback();

private:
    void perform(bool do_commit);

    dbm::session<Impl>& db_;
    bool executed_ {false};
};

}// namespace dbm

#endif//DBM_SESSION_HPP
