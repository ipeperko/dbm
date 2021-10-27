#ifndef DBM_SESSION_IPP
#define DBM_SESSION_IPP

namespace dbm {

DBM_INLINE session::transaction::transaction(session& db)
    : db_(db)
{
    db_.transaction_begin();
}

DBM_INLINE session::transaction::~transaction()
{
    perform(false);
}

DBM_INLINE void session::transaction::commit()
{
    perform(true);
}

DBM_INLINE void session::transaction::rollback()
{
    perform(false);
}

DBM_INLINE void session::transaction::perform(bool do_commit)
{
    if (!executed_) {
        if (do_commit) {
            db_.transaction_commit();
        }
        else {
            db_.transaction_rollback();
        }
        executed_ = true;
    }
}

DBM_INLINE session::session(const session& oth)
    : last_statement_(oth.last_statement_)
{
}

DBM_INLINE session::session(session&& oth) noexcept
    : last_statement_(std::move(oth.last_statement_))
{
}

DBM_INLINE session& session::operator=(const session& oth)
{
    if (this != &oth) {
        last_statement_ = oth.last_statement_;
    }
    return *this;
}

DBM_INLINE session& session::operator=(session&& oth) noexcept
{
    if (this != &oth) {
        last_statement_ = std::move(oth.last_statement_);
    }
    return *this;
}

DBM_INLINE void session::query(const std::string& statement)
{
    last_statement_ = statement;
}

DBM_INLINE void session::query(const detail::statement& q)
{
    query(q.get());
}

DBM_INLINE kind::sql_rows session::select(const std::string& statement)
{
    return select_rows(statement);
}

DBM_INLINE kind::sql_rows session::select(const std::vector<std::string>& what, const std::string& table, const std::string& criteria)
{
    std::string& statement = last_statement_;
    statement = "SELECT ";

    int i = 0;
    for (auto it = what.begin(); it != what.end(); it++, i++) {
        statement += *it;
        if (i < static_cast<int>(what.size()) - 1) {
            statement += ", ";
        }
    }

    statement += " FROM ";
    statement += table;

    if (!criteria.empty()) {
        statement += " " + criteria;
    }

    return select_rows(statement);
}

DBM_INLINE kind::sql_rows session::select(const detail::statement& q)
{
    return select(q.get());
}

DBM_INLINE void session::remove_prepared_statement(std::string const& s)
{
    auto p = prepared_stm_handle_.find(s);
    if (p != prepared_stm_handle_.end())
        prepared_stm_handle_.erase(p);
}

DBM_INLINE void session::create_database(const std::string& db_name, bool if_not_exists)
{
    std::string q = "CREATE DATABASE ";
    if (if_not_exists) {
        q += "IF NOT EXISTS ";
    }
    q += db_name;
    query(q);
}

DBM_INLINE void session::drop_database(const std::string& db_name, bool if_exists)
{
    std::string q = "DROP DATABASE ";
    if (if_exists) {
        q += "IF EXISTS ";
    }
    q += db_name;
    query(q);
}

DBM_INLINE void session::create_table(const model& m, bool if_not_exists)
{
    std::string q = create_table_query(m, if_not_exists);
    query(q);
}

DBM_INLINE void session::drop_table(const model& m, bool if_exists)
{
    std::string q = drop_table_query(m, if_exists);
    query(q);
}

DBM_INLINE std::string session::last_statement_info() const
{
    return " - statement : " + last_statement_;
}

DBM_INLINE model& session::operator>>(model& m)
{
    m.read_record(*this);
    return m;
}

DBM_INLINE model&& session::operator>>(model&& m)
{
    m.read_record(*this);
    return std::move(m);
}

} // namespace dbm

#endif //DBM_SESSION_IPP
