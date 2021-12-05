#ifdef DBM_SQLITE3

#include <dbm/drivers/sqlite/sqlite_session.hpp>
#include <dbm/dbm_common.hpp>

#include <dbm/model_item.hpp>
#include <dbm/model.hpp>
#include <dbm/impl/model_item.ipp>
#include <dbm/impl/model.ipp>
#include <dbm/impl/session.ipp>
#include <dbm/detail/model_query_helper.hpp>
#include <cstring>
#include <sqlite3.h>

namespace dbm {

namespace {

class error_message
{
public:
    ~error_message()
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

    friend std::ostream& operator<<(std::ostream& os, error_message const& msg)
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

std::unique_ptr<session> sqlite_session::clone() const
{
    return std::make_unique<sqlite_session>(*this);
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
    int rc = sqlite3_open(db_name_.c_str(), &db3_);
    if (rc) {
        throw_exception<std::runtime_error>("SQLite can't open database " + db_name_);
    }
}

void sqlite_session::close()
{
    free_table();
    destroy_prepared_stmt_handles();

    if (db3_) {
        sqlite3_close(db3_);
        db3_ = nullptr;
    }
}

void sqlite_session::query(const std::string& statement)
{
    error_message zErrMsg;

    last_statement_ = statement;

    int rc = sqlite3_exec(db3_, statement.c_str(), nullptr, nullptr, zErrMsg.ptr());
    if (rc != SQLITE_OK) {
        // TODO: check connection lost
        throw_exception<std::runtime_error>("SQLite error " + zErrMsg.to_string() + last_statement_info());
    }
}

kind::sql_rows sqlite_session::select_rows(const std::string& statement)
{
    int rc;
    int nRow;
    int nColumn;
    error_message zErrMsg;
    kind::sql_rows rows;

    last_statement_ = statement;

    free_table();

    /* query */
    rc = sqlite3_get_table(db3_, statement.c_str(), &azResult, &nRow, &nColumn, zErrMsg.ptr());
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
            r.emplace_back(azResult[i], azResult[i] ? strlen(azResult[i]) : 0);
            i++;
        }
    }

    return rows;
}

void sqlite_session::init_prepared_statement(kind::prepared_statement& stmt)
{
    if (stmt.native_handle()) {
        // check if handle is registered
        if (prepared_stm_handle_.end() != std::find_if(
                prepared_stm_handle_.begin(),
                prepared_stm_handle_.end(),
                [hdl = stmt.native_handle()] (const std::pair<std::string, void*>& ch) {
                    return ch.second == hdl; }))
        {
            // handle exists in cache
            return;
        }
        else {
            // handle doesn't exist
            kind::prepared_statement_manipulator(stmt).set_native_handle(nullptr);
        }
    }

    if (prepared_stm_handle_.find(stmt.statement()) != prepared_stm_handle_.end()) {
        kind::prepared_statement_manipulator(stmt).set_native_handle(prepared_stm_handle_[stmt.statement()]);
        return;
    }

    sqlite3_stmt *pStmt;
    int rc = sqlite3_prepare_v2(db3_, stmt.statement().c_str(), static_cast<int>(stmt.statement().size()), &pStmt, nullptr);

    if (rc != SQLITE_OK)
        throw_exception("SQLite prepare statement failed : " + std::string(sqlite3_errmsg(db3_)));

    prepared_stm_handle_[stmt.statement()] = pStmt;
    kind::prepared_statement_manipulator(stmt).set_native_handle(pStmt);
}

void sqlite_session::query(kind::prepared_statement& stmt)
{
    if (!db3_)
        throw_exception("SQLite connection is closed");

    free_table();

    if (!stmt.native_handle())
        init_prepared_statement(stmt);

    auto handle = reinterpret_cast<sqlite3_stmt*>(stmt.native_handle());

    utils::execute_at_exit finally([&handle] {
        sqlite3_reset(handle);
    });

    int const nparams = static_cast<int>(stmt.size());

    for (int i = 0; i < nparams; ++i) {

        container* p = stmt.param(i);
        int rc = SQLITE_ERROR;
        int col_idx = i + 1;

        if (p->is_null()) {
            rc = sqlite3_bind_null(handle, col_idx);
        }
        else {
            switch (p->type()) {
                case kind::data_type::Nullptr:
                    rc = sqlite3_bind_null(handle, col_idx);
                    break;
                case kind::data_type::Bool:
                    rc = sqlite3_bind_int(handle, col_idx, p->get<bool>());
                    break;
                case kind::data_type::Int32:
                    rc = sqlite3_bind_int(handle, col_idx, p->get<int32_t>());
                    break;
                case kind::data_type::Int16:
                    rc = sqlite3_bind_int(handle, col_idx, p->get<int16_t>());
                    break;
                case kind::data_type::Int64:
                    rc = sqlite3_bind_int64(handle, col_idx, p->get<int64_t>());
                    break;
                case kind::data_type::UInt32:
                    rc = sqlite3_bind_int(handle, col_idx, p->get<uint32_t>());
                    break;
                case kind::data_type::UInt16:
                    rc = sqlite3_bind_int(handle, col_idx, p->get<uint16_t>());
                    break;
                case kind::data_type::UInt64:
                    rc = sqlite3_bind_int64(handle, col_idx, p->get<uint64_t>()); // narrowing conversion?
                    break;
                case kind::data_type::Double:
                    rc = sqlite3_bind_double(handle, col_idx, p->get<double>());
                    break;
                case kind::data_type::String:
                    {
                        auto* text = reinterpret_cast<std::string*>(p->buffer());
                        rc = sqlite3_bind_text(handle, col_idx, text->c_str(), static_cast<int>(text->size()), SQLITE_STATIC);
                    }
                    break;
                default:
                    throw_exception("SQLite prepared statement type not supported");
            }
        }

        if (rc != SQLITE_OK)
            throw_exception("SQLite bind error : " + std::string(sqlite3_errmsg(db3_)));
    }

    int rc = sqlite3_step(handle);

    if (rc != SQLITE_DONE)
        throw_exception("SQLite step error : " + std::string(sqlite3_errmsg(db3_)));
}

std::vector<std::vector<container_ptr>> sqlite_session::select(dbm::kind::prepared_statement& stmt)
{
    if (!db3_)
        throw_exception("SQLite connection is closed");

    free_table();

    if (!stmt.native_handle())
        init_prepared_statement(stmt);

    auto handle = reinterpret_cast<sqlite3_stmt*>(stmt.native_handle());

    utils::execute_at_exit finally([&handle] {
        sqlite3_reset(handle);
    });

    std::vector<std::vector<container_ptr>> rows;
    int const nparams = static_cast<int>(stmt.size());
    int rc;

    while (true) {
        rc = sqlite3_step(handle);

        if (rc == SQLITE_ROW) {
            std::vector<container_ptr> row;

            for (int i = 0; i < nparams; ++i) {

                int col_idx = i;
                container const* psrc = stmt.param(i);
                row.push_back(local_container_factory(psrc->type()));
                auto& pdest = row.back();

                rc = sqlite3_column_type(handle, col_idx);

                if (rc == SQLITE_NULL) {
                    row.back()->set_null(true);
                }
                else {
                    switch (psrc->type()) {
                        case kind::data_type::Bool:
                        case kind::data_type::Int32:
                        case kind::data_type::Int16:
                        case kind::data_type::UInt16:
                        case kind::data_type::UInt32:
                            pdest->set(sqlite3_column_int(handle, col_idx));
                            break;
                        case kind::data_type::Int64:
                        case kind::data_type::UInt64:
                            pdest->set(static_cast<int64_t>(sqlite3_column_int64(handle, col_idx)));
                            break;
                        case kind::data_type::Double:
                            pdest->set(sqlite3_column_double(handle, col_idx));
                            break;
                        case kind::data_type::String:
                            {
                                //int bytes = sqlite3_column_bytes(handle, col_idx);
                                auto* txt = reinterpret_cast<const char*>(sqlite3_column_text(handle, col_idx));
                                pdest->set(txt);
                            }
                            break;
                        default:
                            throw_exception("SQLite prepared statement type not supported");
                    }
                }
            }

            rows.push_back(std::move(row));
        }
        else if (rc == SQLITE_DONE) {
            break;
        }
        else {
            throw_exception("SQLite step error : " + std::string(sqlite3_errmsg(db3_)));
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

void sqlite_session::transaction_begin()
{
    query("BEGIN");
}

void sqlite_session::transaction_commit()
{
    query("COMMIT");
}
void sqlite_session::transaction_rollback()
{
    query("ROLLBACK");
}


void sqlite_session::free_table()
{
    if (azResult) {
        sqlite3_free_table(azResult);
        azResult = nullptr;
    }
}

void sqlite_session::destroy_prepared_stmt_handles()
{
    for (auto& h : prepared_stm_handle_) {
        sqlite3_finalize(reinterpret_cast<sqlite3_stmt*>(h.second));
    }

    prepared_stm_handle_.clear();
}

}// namespace dbm

#endif
