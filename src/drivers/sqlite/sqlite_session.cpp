#ifdef DBM_SQLITE3

#include <dbm/drivers/sqlite/sqlite_session.hpp>
#include <dbm/model.hpp>
#include <dbm/model.ipp>
#include <dbm/session.ipp>
#include <dbm/detail/model_query_helper.hpp>
#include <cstring>
#include <sqlite3.h>

namespace dbm {

namespace {
class Stmt
{
public:
    ~Stmt()
    {
        reset();
    }

    void reset()
    {
        if (s_) {
            sqlite3_finalize(s_);
            s_ = nullptr;
        }
    }

    sqlite3_stmt** ptr()
    {
        return &s_;
    }

private:
    sqlite3_stmt* s_ {nullptr};
};


class ErrMsg
{
public:
    ~ErrMsg()
    {
        reset();
    }

    void reset()
    {
        if (p_) {
            sqlite3_free(p_);
            p_ = nullptr;
        }
    }

    char** ptr()
    {
        return &p_;
    }

    friend std::ostream& operator<<(std::ostream& os, ErrMsg const& msg)
    {
        os << msg.p_;
        return os;
    };

    std::string to_string() const
    {
        return p_;
    }

private:
    char* p_ {nullptr};
};

}// namespace

sqlite_session::sqlite_session()
{
}

sqlite_session::sqlite_session(std::string_view db_name)
    : db_name_(db_name)
{
}

sqlite_session::sqlite_session(const sqlite_session& oth)
    : session(oth)
    , db_name_(oth.db_name_)
{
}

sqlite_session& sqlite_session::operator=(const sqlite_session& oth)
{
    if (this != &oth) {
        session::operator=(oth);
        db_name_ = oth.db_name_;
    }
    return *this;
}

sqlite_session::~sqlite_session()
{
    sqlite_session::close();
}

void sqlite_session::set_db_name(std::string_view file_name)
{
    db_name_ = file_name;
}

std::string_view sqlite_session::db_name() const
{
    return db_name_;
}

void sqlite_session::open()
{
    int rc = sqlite3_open(db_name_.c_str(), &db3);
    if (rc) {
        throw_exception<std::runtime_error>("Can't open database " + db_name_);
    }
}

void sqlite_session::close()
{
    free_table();

    if (db3) {
        sqlite3_close(db3);
        db3 = nullptr;
    }
}

void sqlite_session::query(const std::string& statement)
{
    ErrMsg zErrMsg;

    mstatement = statement;

    int rc = sqlite3_exec(db3, statement.c_str(), nullptr, nullptr, zErrMsg.ptr());
    if (rc != SQLITE_OK) {
        throw_exception<std::runtime_error>("SQLite error " + zErrMsg.to_string() + last_statement_info());
    }
}

kind::sql_rows sqlite_session::select_rows(const std::string& statement)
{
    int rc;
    int nRow;
    int nColumn;
    ErrMsg zErrMsg;
    kind::sql_rows rows;

    mstatement = statement;

    free_table();

    /* query */
    rc = sqlite3_get_table(db3, statement.c_str(), &azResult, &nRow, &nColumn, zErrMsg.ptr());
    if (rc != SQLITE_OK) {
        throw_exception<std::runtime_error>("SQLite error " + zErrMsg.to_string() + last_statement_info());
    }

    /* field names */
    {
        kind::sql_fields fields_tmp;
        fields_tmp.reserve(nColumn);
        for (int ci = 0; ci < nColumn; ci++) {
            fields_tmp.emplace_back(azResult[ci]);
        }

        /* setup rows object field names */
        rows.set_field_names(std::move(fields_tmp));
    }

    /* fetch rows */
    int i = nColumn;
    for (int ri = 1; ri < nRow + 1; ri++) {

        kind::sql_row& r = rows.emplace_back();
        r.set_fields(&rows.field_names(), &rows.field_map());

        for (int ci = 0; ci < nColumn; ci++) {
            r.emplace_back(azResult[i], azResult[i] ? strlen(azResult[i]) : 0); // TODO: strlen ?
            i++;
        }
    }

    return rows;
}

std::string sqlite_session::write_model_query(const model& m) const
{
    return detail::model_query_helper<detail::param_session_type_sqlite>(m).write_query();
}

std::string sqlite_session::read_model_query(const model& m, const std::string& extra_condition) const
{
    return detail::model_query_helper<detail::param_session_type_sqlite>(m).read_query(extra_condition);
}

std::string sqlite_session::delete_model_query(const model& m) const
{
    return detail::model_query_helper<detail::param_session_type_sqlite>(m).delete_query();
}

std::string sqlite_session::create_table_query(const model& m, bool if_not_exists) const
{
    return detail::model_query_helper<detail::param_session_type_sqlite>(m).create_table_query(if_not_exists);
}

std::string sqlite_session::drop_table_query(const model& m, bool if_exists) const
{
    return detail::model_query_helper<detail::param_session_type_sqlite>(m).drop_table_query(if_exists);
}

void sqlite_session::free_table()
{
    if (azResult) {
        sqlite3_free_table(azResult);
        azResult = nullptr;
    }
}

}// namespace dbm

#endif