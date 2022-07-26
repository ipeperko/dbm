#ifdef DBM_MYSQL

#include <dbm/drivers/mysql/mysql_session.hpp>
#include <dbm/dbm_common.hpp>
#include <dbm/model_item.hpp>
#include <dbm/model.hpp>
#include <dbm/impl/model_item.ipp>
#include <dbm/impl/model.ipp>
#include <dbm/impl/session.ipp>
#include <dbm/detail/model_query_helper.hpp>

#ifdef __WIN32
#include <mysql.h>
#include <windows.h>
#endif
#ifdef __linux
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#endif

#include <cstring>

#define MYSQL_CONNECTION_HANDLE (reinterpret_cast<MYSQL*>(conn_))
#define MYSQL_RES_HANDLE (reinterpret_cast<MYSQL_RES*>(res_set_))

namespace dbm {

mysql_session::mysql_session(const mysql_session& oth)
    : session(oth)
{
    *this = oth;
}

mysql_session::mysql_session(mysql_session&& oth) noexcept
{
    *this = std::move(oth);
}

mysql_session& mysql_session::operator=(const mysql_session& oth)
{
    if (this != &oth) {
        // __conn cannot copy
        session::operator=(oth);
    }
    return *this;
}

mysql_session& mysql_session::operator=(mysql_session&& oth) noexcept
{
    if (this != &oth) {
        close();
        conn_ = oth.conn_;
        oth.conn_ = nullptr;
        session::operator=(std::move(oth));
    }
    return *this;
}

mysql_session::~mysql_session()
{
    mysql_session::close();
}

void mysql_session::connect(
    std::optional<std::string_view> host,
    std::optional<std::string_view> user,
    std::optional<std::string_view> passwd,
    std::optional<std::string_view> db,
    unsigned int port,
    std::optional<std::string_view> unix_socket,
    unsigned long client_flag)
{
    if (conn_) {
        return;
    }

    /* initialize connection handler */
    conn_ = mysql_init(nullptr);

    if (!conn_) {
        throw_exception<std::runtime_error>("MySql connection failed");
    }

    /* connect to server */
    MYSQL* s = mysql_real_connect(MYSQL_CONNECTION_HANDLE,
                                  host ? host.value().data() : nullptr,
                                  user ? user.value().data() : nullptr,
                                  passwd ? passwd.value().data() : nullptr,
                                  db ? db.value().data() : nullptr,
                                  port,
                                  unix_socket ? unix_socket.value().data() : nullptr,
                                  client_flag);
    if (s == nullptr) {
        conn_ = nullptr;
        throw_exception<std::runtime_error>("MySql cannot connect to database " + std::string(db ? db.value().data() : ""));
    }
}

void mysql_session::close_impl()
{
    free_result_set();

    if (MYSQL_CONNECTION_HANDLE) {
        for (auto& h : prepared_stm_handle_) {
            mysql_stmt_close(reinterpret_cast<MYSQL_STMT*>(h.second));
        }

        mysql_close(MYSQL_CONNECTION_HANDLE);
        conn_ = nullptr;
    }

    prepared_stm_handle_.clear();
}

void mysql_session::query_impl(std::string_view statement)
{
    last_statement_ = statement;

    if (!MYSQL_CONNECTION_HANDLE)
        throw_exception<std::runtime_error>("MySQL connection not established!");

    /* free result set if it holds any data */
    free_result_set();

    /* query */
    if (mysql_real_query(MYSQL_CONNECTION_HANDLE, statement.data(), statement.length())) {
        auto errn = mysql_errno(MYSQL_CONNECTION_HANDLE);
        const char* errmsg = mysql_error(MYSQL_CONNECTION_HANDLE);

        if (errn == CR_SERVER_GONE_ERROR || errn == CR_SERVER_LOST) {
            /* connection lost */
            close();
        }

        throw_exception<std::runtime_error>(std::string("MySql query error : ")
                                            + errmsg
                                            + std::string(" (") + std::to_string(errn)
                                            + std::string(")") + last_statement_info());
    }
}

kind::sql_rows mysql_session::select_rows_impl(std::string_view statement)
{
    kind::sql_rows rows;
    unsigned num_fields = 0;

    /* execute query */
    query(statement);

    /* grab the result */
    res_set_ = mysql_store_result(MYSQL_CONNECTION_HANDLE);
    if (!res_set_) {
        if (mysql_field_count(MYSQL_CONNECTION_HANDLE) > 0) {
            throw_exception<std::runtime_error>(std::string("Error processing result set : ") + mysql_error(MYSQL_CONNECTION_HANDLE) + last_statement_info());
        }
        else {
            throw_exception<std::runtime_error>(std::string("Error rows affected : ") + mysql_error(MYSQL_CONNECTION_HANDLE) + last_statement_info());
        }
    }

    /* field names */
    {
        mysql_field_seek(MYSQL_RES_HANDLE, 0);
        num_fields = mysql_num_fields(MYSQL_RES_HANDLE);
        kind::sql_fields fields_tmp;
        fields_tmp.reserve(num_fields);

        for (unsigned i = 0; i < num_fields; i++) {
            MYSQL_FIELD* field = mysql_fetch_field(MYSQL_RES_HANDLE);
            fields_tmp.push_back(field->name);
        }

        /* setup rows object field names */
        rows.set_field_names(std::move(fields_tmp));
    }

    /* fetch rows */
    MYSQL_ROW mysql_row;
    while ((mysql_row = mysql_fetch_row(MYSQL_RES_HANDLE)) != nullptr) {

        kind::sql_row& r = rows.emplace_back();
        r.set_fields(&rows.field_names(), &rows.field_map());
        r.reserve(num_fields);

        unsigned long* lenghts = mysql_fetch_lengths(MYSQL_RES_HANDLE);

        for (unsigned i = 0; i < num_fields; i++) {
            r.emplace_back(mysql_row[i], lenghts[i]);
        }
    }

    /* check error */
    if (mysql_errno(MYSQL_CONNECTION_HANDLE) != 0) {
        throw_exception<std::runtime_error>("Fetch row error");
    }

    //    sql.check_errors();
    return rows;
}

void mysql_session::init_prepared_statement_impl(kind::prepared_statement& stmt)
{
    if (stmt.native_handle())
        return;

    if (prepared_stm_handle_.find(stmt.statement()) != prepared_stm_handle_.end()) {
        kind::prepared_statement_manipulator(stmt).set_native_handle(prepared_stm_handle_[stmt.statement()]);
        return;
    }

    auto stmt_handle = mysql_stmt_init(MYSQL_CONNECTION_HANDLE);
    if (!stmt_handle)
        throw_exception("MySql init statement failed + " + last_mysql_error());

    // TODO: should this be executed after mysql_stmt_prepare?
    prepared_stm_handle_[stmt.statement()] = stmt_handle;
    kind::prepared_statement_manipulator(stmt).set_native_handle(stmt_handle);

    if (mysql_stmt_prepare(stmt_handle, stmt.statement().c_str(), stmt.statement().length()))
        throw_exception("MySql prepare statement failed : " + std::string() + mysql_stmt_error(stmt_handle));
}

void mysql_session::query_impl(kind::prepared_statement& stmt)
{
    using bool_type = std::remove_pointer<decltype(MYSQL_BIND::is_null)>::type;

    if (!MYSQL_CONNECTION_HANDLE)
        throw_exception<std::runtime_error>("MySQL connection not established!");

    if (!stmt.native_handle())
        init_prepared_statement(stmt);

    auto handle = reinterpret_cast<MYSQL_STMT*>(stmt.native_handle());

    /* free result set if it holds any data */
    free_result_set();

    auto nparams = stmt.size();
    MYSQL_BIND bind[nparams];
    bool_type isNull[nparams];
    memset(bind, 0, sizeof(MYSQL_BIND) * nparams);

    for (auto i = 0u; i < nparams; ++i) {

        container* p = stmt.param(i);
        MYSQL_BIND& b = bind[i];
        isNull[i] = static_cast<bool_type>(p->is_null());

        b.buffer = p->buffer();
        b.is_null = &isNull[i];
        b.buffer_length = 0;

        switch (p->type()) {
            case kind::data_type::Nullptr:
                b.buffer_type = MYSQL_TYPE_NULL;
                break;
            case kind::data_type::Bool:
                b.buffer_type = MYSQL_TYPE_TINY;
                break;
            case kind::data_type::Int32:
                b.buffer_type = MYSQL_TYPE_LONG;
                break;
            case kind::data_type::Int16:
                b.buffer_type = MYSQL_TYPE_SHORT;
                break;
            case kind::data_type::Int64:
                b.buffer_type = MYSQL_TYPE_LONGLONG;
                break;
            case kind::data_type::UInt32:
                b.buffer_type = MYSQL_TYPE_LONG;
                b.is_unsigned = true;
                break;
            case kind::data_type::UInt16:
                b.buffer_type = MYSQL_TYPE_SHORT;
                b.is_unsigned = true;
                break;
            case kind::data_type::UInt64:
                b.buffer_type = MYSQL_TYPE_LONGLONG;
                b.is_unsigned = true;
                break;
            case kind::data_type::Double:
                b.buffer_type = MYSQL_TYPE_DOUBLE;
                break;
            case kind::data_type::String:
                b.buffer = (void*)reinterpret_cast<std::string*>(p->buffer())->c_str();
                b.buffer_type = MYSQL_TYPE_STRING;
                b.buffer_length = p->length();
                break;
            default:
                throw_exception("MySql prepared statement type not supported"); // TODO: handle other types
        }
    }

    if (mysql_stmt_bind_param(handle, bind))
        throw_exception("MySql prepared statement bind param failed : " + std::string() + mysql_stmt_error(handle));

    if (mysql_stmt_execute(handle))
        throw_exception("MySql prepared statement execute failed (query) : " + std::string() + mysql_stmt_error(handle));

    /* store result should be performed before executing new query only if we are retrieving result (select ...) */
    if (mysql_stmt_store_result(handle))
        throw_exception("MySql prepared statement store result failed : " + std::string() + mysql_stmt_error(handle));
}

std::vector<std::vector<container_ptr>> mysql_session::select_impl(dbm::kind::prepared_statement& stmt)
{
    using bool_type = std::remove_pointer<decltype(MYSQL_BIND::is_null)>::type;
    using string_length_type = std::remove_pointer<decltype(MYSQL_BIND::length)>::type;

    if (!MYSQL_CONNECTION_HANDLE)
        throw_exception<std::runtime_error>("MySQL connection not established!");

    if (!stmt.native_handle())
        init_prepared_statement(stmt);

    auto handle = reinterpret_cast<MYSQL_STMT*>(stmt.native_handle());

    /* free result set if it holds any data */
    free_result_set();

    MYSQL_BIND bind[stmt.size()];
    bool_type isNull[stmt.size()];
    memset(bind, 0, sizeof(MYSQL_BIND) * stmt.size());

    std::vector<size_t> string_column;
    std::vector<std::string*> string_buffer;
    std::vector<string_length_type> string_len;

    for (auto i = 0u; i < stmt.size(); ++i)
    {
        auto p = stmt.param(i);
        MYSQL_BIND& b = bind[i];
        isNull[i] = static_cast<bool_type>(p->is_null());

        b.buffer = p->buffer();
        b.is_null = &isNull[i];
        b.buffer_length = 0;

        switch (p->type()) {
            case kind::data_type::Nullptr:
                b.buffer_type = MYSQL_TYPE_NULL;
                break;
            case kind::data_type::Bool:
                b.buffer_type = MYSQL_TYPE_TINY;
                break;
            case kind::data_type::Int32:
                b.buffer_type = MYSQL_TYPE_LONG;
                break;
            case kind::data_type::Int16:
                b.buffer_type = MYSQL_TYPE_SHORT;
                break;
            case kind::data_type::Int64:
                b.buffer_type = MYSQL_TYPE_LONGLONG;
                break;
            case kind::data_type::UInt32:
                b.buffer_type = MYSQL_TYPE_LONG;
                b.is_unsigned = true;
                break;
            case kind::data_type::UInt16:
                b.buffer_type = MYSQL_TYPE_SHORT;
                b.is_unsigned = true;
                break;
            case kind::data_type::UInt64:
                b.buffer_type = MYSQL_TYPE_LONGLONG;
                b.is_unsigned = true;
                break;
            case kind::data_type::Double:
                b.buffer_type = MYSQL_TYPE_DOUBLE;
                break;
            case kind::data_type::String:
                string_column.push_back(i);
                string_buffer.push_back(reinterpret_cast<std::string*>(p->buffer()));
                string_len.push_back(0);

                b.buffer = nullptr;
                b.buffer_type = MYSQL_TYPE_STRING;
                b.buffer_length = 0;
                b.length = &string_len.back();
                break;
            default:
                throw_exception("MySql prepared statement type not supported"); // TODO: handle other types
        }
    }

    if (mysql_stmt_bind_result(handle, bind))
        throw_exception("MySql prepared statement bind param failed : " + std::string() + mysql_stmt_error(handle));

    if (mysql_stmt_execute(handle))
        throw_exception("MySql prepared statement execute failed (select) : " + std::string() + mysql_stmt_error(handle));

    // TODO: store result optional?
    /*
    if (mysql_stmt_store_result(handle))
        throw_exception("MySql prepared statement store result failed : " + std::string() + mysql_stmt_error(handle));
    */

    std::vector<std::vector<container_ptr>> rows;

    while(true) {
        int result = mysql_stmt_fetch(handle);

        if (result == MYSQL_NO_DATA)
            break;

        for (auto i = 0u; i < string_column.size(); i++) {
            auto col = string_column[i];
            auto* buf = string_buffer[i];

            buf->resize(string_len[i]);
            bind[col].buffer = buf->data();
            bind[col].buffer_length = buf->length();

            if (mysql_stmt_fetch_column(handle, &bind[col], col, 0))
                throw_exception("MySql prepared statement fetch column failed : " + std::string() + mysql_stmt_error(handle));

            bind[col].buffer = nullptr;
            bind[col].buffer_length = 0;
        }

        std::vector<container_ptr> row;

        for (auto i = 0u; i < stmt.size(); ++i) {
            auto* srcp = stmt.param(i);
            srcp->set_null(isNull[i]);
            row.push_back(local_container_factory(srcp->type()));
            if (isNull[i]) {
                row.back()->set_null(true);
            }
            else {
                row.back()->set_null(false);
                row.back()->set(srcp->get());
            }
        }

        rows.push_back(std::move(row));
    }

    return rows;
}

std::string mysql_session::write_model_query_impl(const model& m) const
{
    return detail::model_query_helper<detail::param_session_type_mysql>(m).write_query();
}

std::string mysql_session::read_model_query_impl(const model& m, const std::string& extra_condition) const
{
    return detail::model_query_helper<detail::param_session_type_mysql>(m).read_query(extra_condition);
}

std::string mysql_session::delete_model_query_impl(const model& m) const
{
    return detail::model_query_helper<detail::param_session_type_mysql>(m).delete_query();
}

std::string mysql_session::create_table_query_impl(const model& m, bool if_not_exists) const
{
    return detail::model_query_helper<detail::param_session_type_mysql>(m).create_table_query(if_not_exists);
}

std::string mysql_session::drop_table_query_impl(const model& m, bool if_exists) const
{
    return detail::model_query_helper<detail::param_session_type_mysql>(m).drop_table_query(if_exists);
}

void mysql_session::transaction_begin_impl()
{
    query("BEGIN");
}

void mysql_session::transaction_commit_impl()
{
    query("COMMIT");
}

void mysql_session::transaction_rollback_impl()
{
    query("ROLLBACK");
}

void mysql_session::free_result_set()
{
    if (res_set_) {
        mysql_free_result(MYSQL_RES_HANDLE);
        res_set_ = nullptr;

        /* this should be called in case of procedures CALL (error 2014) */
        while (mysql_next_result(MYSQL_CONNECTION_HANDLE) == 0) {
            res_set_ = mysql_store_result(MYSQL_CONNECTION_HANDLE);
            if (res_set_) {
                mysql_free_result(MYSQL_RES_HANDLE);
                res_set_ = nullptr;
            }
        }
    }
}

std::string mysql_session::last_mysql_error() const
{
    if (MYSQL_CONNECTION_HANDLE)
        return { mysql_error(MYSQL_CONNECTION_HANDLE) };

    return {};
}

}// namespace dbm

#endif
