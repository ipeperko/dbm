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
    : mstatement(oth.mstatement)
{
}

DBM_INLINE session::session(session&& oth) noexcept
    : mstatement(std::move(oth.mstatement))
{
}

DBM_INLINE session& session::operator=(const session& oth)
{
    if (this != &oth) {
        mstatement = oth.mstatement;
    }
    return *this;
}

DBM_INLINE session& session::operator=(session&& oth) noexcept
{
    if (this != &oth) {
        mstatement = std::move(oth.mstatement);
    }
    return *this;
}

DBM_INLINE void session::query(const std::string& statement)
{
    mstatement = statement;
}

DBM_INLINE kind::sql_rows session::select(const std::string& statement)
{
    return select_rows(statement);
}

DBM_INLINE kind::sql_rows session::select(const std::vector<std::string>& what, const std::string& table, const std::string& criteria)
{
    std::string& statement = mstatement;
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

DBM_INLINE void session::create_table(const std::string& tbl_name, std::vector<std::string> fields, bool if_not_exists)
{
    std::string q = "CREATE TABLE ";
    if (if_not_exists) {
        q += "IF NOT EXISTS ";
    }
    q += tbl_name + " (";

    // TODO: empty list exception

    unsigned i = 0;

    for (auto const& field : fields) {
        if (i) {
            q += ", ";
        }
        q += field;
        ++i;
    }

    q += ")";
    query(q);
}

DBM_INLINE void session::drop_table(const model& m, bool if_exists)
{
    std::string q = drop_table_query(m, if_exists);
    query(q);
}

DBM_INLINE void session::drop_table(const std::string& tbl_name, bool if_exists)
{
    std::string q = "DROP TABLE ";
    if (if_exists) {
        q += "IF EXISTS ";
    }
    q += tbl_name;
    query(q);
}

DBM_INLINE std::string session::last_statement_info() const
{
    return " - statement : " + mstatement;
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
