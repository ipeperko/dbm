#include "session.hpp"
#include "model.hpp"

namespace dbm {

session::session(const session& oth)
    : mstatement(oth.mstatement)
{
}

session::session(session&& oth) noexcept
    : mstatement(std::move(oth.mstatement))
{
}

session& session::operator=(const session& oth)
{
    if (this != &oth) {
        mstatement = oth.mstatement;
    }
    return *this;
}

session& session::operator=(session&& oth) noexcept
{
    if (this != &oth) {
        mstatement = std::move(oth.mstatement);
    }
    return *this;
}

void session::query(const std::string& statement)
{
    mstatement = statement;
}

kind::sql_rows session::select(const std::vector<std::string>& what, const std::string& table, const std::string& criteria)
{
    std::string& statement = mstatement;
    statement = "SELECT ";

    int i = 0;
    for (auto it = what.begin(); it != what.end(); it++, i++) {
        statement += *it;
        if (i < (int)what.size() - 1) {
            statement += ", ";
        }
    }

    statement += " FROM ";
    statement += table;

    if (!criteria.empty()) {
        statement += " " + criteria;
    }

    return select(statement);
}

void session::create_database(const std::string& db_name, bool if_not_exists)
{
    std::string q = "CREATE DATABASE ";
    if (if_not_exists) {
        q += "IF NOT EXISTS ";
    }
    q += db_name;
    query(q);
}

void session::drop_database(const std::string& db_name, bool if_exists)
{
    std::string q = "DROP DATABASE ";
    if (if_exists) {
        q += "IF EXISTS ";
    }
    q += db_name;
    query(q);
}

void session::create_table(const model& m, bool if_not_exists)
{
    std::string q = create_table_query(m, if_not_exists);
    query(q);
}

void session::create_table(const std::string& tbl_name, std::vector<std::string> fields, bool if_not_exists)
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

void session::drop_table(const model& m, bool if_exists)
{
    std::string q = drop_table_query(m, if_exists);
    query(q);
}

void session::drop_table(const std::string& tbl_name, bool if_exists)
{
    std::string q = "DROP TABLE ";
    if (if_exists) {
        q += "IF EXISTS ";
    }
    q += tbl_name;
    query(q);
}

std::string session::last_statement_info() const
{
    return " - statement : " + mstatement;
}

model& session::operator>>(model& m)
{
    m.read_record(*this);
    return m;
}

model&& session::operator>>(model&& m)
{
    m.read_record(*this);
    return std::move(m);
}


} // namespace dbm