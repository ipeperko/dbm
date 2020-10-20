#ifndef DBM_SESSION_IPP
#define DBM_SESSION_IPP

namespace dbm {

inline void session::query(const std::string& statement)
{
    mstatement = statement;
}

inline kind::sql_rows session::select(const std::string& statement)
{
    return select_rows(statement);
}

inline kind::sql_rows session::select(const std::vector<std::string>& what, const std::string& table, const std::string& criteria)
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

inline void session::create_database(const std::string& db_name, bool if_not_exists)
{
    std::string q = "CREATE DATABASE ";
    if (if_not_exists) {
        q += "IF NOT EXISTS ";
    }
    q += db_name;
    query(q);
}

inline void session::drop_database(const std::string& db_name, bool if_exists)
{
    std::string q = "DROP DATABASE ";
    if (if_exists) {
        q += "IF EXISTS ";
    }
    q += db_name;
    query(q);
}

inline void session::create_table(const model& m, bool if_not_exists)
{
    std::string q = create_table_query(m, if_not_exists);
    query(q);
}

inline void session::create_table(const std::string& tbl_name, std::vector<std::string> fields, bool if_not_exists)
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

inline void session::drop_table(const model& m, bool if_exists)
{
    std::string q = drop_table_query(m, if_exists);
    query(q);
}

inline void session::drop_table(const std::string& tbl_name, bool if_exists)
{
    std::string q = "DROP TABLE ";
    if (if_exists) {
        q += "IF EXISTS ";
    }
    q += tbl_name;
    query(q);
}

inline std::string session::last_statement_info() const
{
    return " - statement : " + mstatement;
}

inline model& session::operator>>(model& m)
{
    m.read_record(*this);
    return m;
}

inline model&& session::operator>>(model&& m)
{
    m.read_record(*this);
    return std::move(m);
}

} // namespace dbm

#endif //DBM_SESSION_IPP
