#ifndef DBM_SESSION_IPP
#define DBM_SESSION_IPP

namespace dbm {

template<typename Impl>
DBM_INLINE session<Impl>::transaction::transaction(session& db)
    : db_(db)
{
    db_.transaction_begin();
}

template<typename Impl>
DBM_INLINE session<Impl>::transaction::~transaction()
{
    perform(false);
}

template<typename Impl>
DBM_INLINE void session<Impl>::transaction::commit()
{
    perform(true);
}

template<typename Impl>
DBM_INLINE void session<Impl>::transaction::rollback()
{
    perform(false);
}

template<typename Impl>
DBM_INLINE void session<Impl>::transaction::perform(bool do_commit)
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

template<typename Impl>
DBM_INLINE session<Impl>::session(const session& oth)
    : last_statement_(oth.last_statement_)
{
}

template<typename Impl>
DBM_INLINE session<Impl>::session(session&& oth) noexcept
    : last_statement_(std::move(oth.last_statement_))
    , prepared_stm_handle_(std::move(oth.prepared_stm_handle_))
{
}

template<typename Impl>
DBM_INLINE session<Impl>& session<Impl>::operator=(const session& oth)
{
    if (this != &oth) {
        last_statement_ = oth.last_statement_;
    }
    return *this;
}

template<typename Impl>
DBM_INLINE session<Impl>& session<Impl>::operator=(session&& oth) noexcept
{
    if (this != &oth) {
        last_statement_ = std::move(oth.last_statement_);
        prepared_stm_handle_ = std::move(oth.prepared_stm_handle_);
    }
    return *this;
}

template<typename Impl>
DBM_INLINE void session<Impl>::query(const std::string& statement)
{
    last_statement_ = statement;
}

template<typename Impl>
DBM_INLINE void session<Impl>::query(const detail::statement& q)
{
    query(q.get());
}

template<typename Impl>
DBM_INLINE kind::sql_rows session<Impl>::select(const std::string& statement)
{
    return select_rows(statement);
}

template<typename Impl>
DBM_INLINE kind::sql_rows session<Impl>::select(const std::vector<std::string>& what, const std::string& table, const std::string& criteria)
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

template<typename Impl>
DBM_INLINE kind::sql_rows session<Impl>::select(const detail::statement& q)
{
    return select(q.get());
}

template<typename Impl>
DBM_INLINE void session<Impl>::remove_prepared_statement(std::string const& s)
{
    auto p = prepared_stm_handle_.find(s);
    if (p != prepared_stm_handle_.end())
        prepared_stm_handle_.erase(p);
}

template<typename Impl>
DBM_INLINE void session<Impl>::create_database(const std::string& db_name, bool if_not_exists)
{
    std::string q = "CREATE DATABASE ";
    if (if_not_exists) {
        q += "IF NOT EXISTS ";
    }
    q += db_name;
    query(q);
}

template<typename Impl>
DBM_INLINE void session<Impl>::drop_database(const std::string& db_name, bool if_exists)
{
    std::string q = "DROP DATABASE ";
    if (if_exists) {
        q += "IF EXISTS ";
    }
    q += db_name;
    query(q);
}

template<typename Impl>
DBM_INLINE void session<Impl>::create_table(const model& m, bool if_not_exists)
{
    std::string q = create_table_query(m, if_not_exists);
    query(q);
}

template<typename Impl>
DBM_INLINE void session<Impl>::drop_table(const model& m, bool if_exists)
{
    std::string q = drop_table_query(m, if_exists);
    query(q);
}

template<typename Impl>
DBM_INLINE std::string session<Impl>::last_statement_info() const
{
    return " - statement : " + last_statement_;
}

template<typename Impl>
DBM_INLINE model& session<Impl>::operator>>(model& m)
{
    m.read_record(*this);
    return m;
}

template<typename Impl>
DBM_INLINE model&& session<Impl>::operator>>(model&& m)
{
    m.read_record(*this);
    return std::move(m);
}

} // namespace dbm

#endif //DBM_SESSION_IPP
