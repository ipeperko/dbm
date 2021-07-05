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

namespace dbm {

namespace {

class mysql_private
{
public:
    explicit mysql_private(MYSQL* _con)
    {
        con = _con;
        res_set = nullptr;
    }

    ~mysql_private()
    {
        if (res_set) {
            mysql_free_result(res_set);
            res_set = nullptr;
        }
    }

    void execute(const std::string& statement)
    {

        /*
        if (con == nullptr) {
            throw "MySQL connection not established!";
        }
        */

        if (mysql_real_query(con, statement.c_str(), statement.length())) {
            throw_exception<std::runtime_error>(std::string("MySql query error : ") + mysql_error(con) + " Statement: " + statement);
        }

        /* grab the result */
        res_set = mysql_store_result(con);
        if (res_set == nullptr) {

            if (mysql_field_count(con) > 0) {
                throw_exception<std::runtime_error>(std::string("Error processing result set : ") + mysql_error(con) + " Statement: " + statement);
            }
            else {
                throw_exception<std::runtime_error>(std::string("Error rows affected : ") + mysql_error(con) + " Statement: " + statement);
            }
        }

        mysql_field_seek(res_set, 0);
        for (unsigned i = 0; i < mysql_num_fields(res_set); i++) {
            MYSQL_FIELD* field = mysql_fetch_field(res_set);
            fieldnames.push_back(field->name);
        }
    }

    int check_errors()
    {
        if (mysql_errno(con) != 0) {
            throw_exception<std::runtime_error>("Fetch row error");
        }

        return 0;
    }

    MYSQL* con;        // pointer to connection handler
    MYSQL_RES* res_set;// holds the result set
    kind::sql_fields fieldnames;
};
}// namespace

mysql_session::mysql_session()
{
}

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
    opt_host_name = oth.opt_host_name;
    opt_user_name = oth.opt_user_name;
    opt_password = oth.opt_password;
    opt_port_num = oth.opt_port_num;
    opt_socket_name = oth.opt_socket_name;
    opt_db_name = oth.opt_db_name;
    opt_flags = oth.opt_flags;
    // __conn cannot copy
    session::operator=(oth);
    return *this;
}

mysql_session& mysql_session::operator=(mysql_session&& oth) noexcept
{
    if (this != &oth) {
        opt_host_name = std::move(oth.opt_host_name);
        opt_user_name = std::move(oth.opt_user_name);
        opt_password = std::move(oth.opt_password);
        opt_port_num = std::move(oth.opt_port_num);
        opt_socket_name = std::move(oth.opt_socket_name);
        opt_db_name = std::move(oth.opt_db_name);
        opt_flags = std::move(oth.opt_flags);
        close();
        conn__ = oth.conn__;
        oth.conn__ = nullptr;
        session::operator=(std::move(oth));
    }
    return *this;
}

mysql_session::~mysql_session()
{
    mysql_session::close();
}

std::unique_ptr<session> mysql_session::clone() const
{
    return std::make_unique<mysql_session>(*this);
};

void mysql_session::init(const std::string& host_name, const std::string& user, const std::string& pass, unsigned int port,
                         const std::string& db_name, unsigned int flags)
{
    opt_host_name = host_name;
    opt_user_name = user;
    opt_password = pass;
    opt_port_num = port;
    opt_db_name = db_name;
    opt_flags = flags;
}

void mysql_session::set_database_name(const std::string& name)
{
    opt_db_name = name;
}

void mysql_session::open()
{
    if (conn__) {
        return;
    }

    /* initialize connection handler */
    conn__ = mysql_init(nullptr);

    if (!conn__) {
        throw_exception<std::runtime_error>("MySql connection failed");
    }

    /* connect to server */
    MYSQL* s = mysql_real_connect(reinterpret_cast<MYSQL*>(conn__),
                                  opt_host_name.c_str(),
                                  opt_user_name.c_str(),
                                  opt_password.c_str(),
                                  opt_db_name.empty() ? nullptr : opt_db_name.c_str(),
                                  opt_port_num,
                                  opt_socket_name,
                                  opt_flags);
    if (s == nullptr) {
        conn__ = nullptr;
        throw_exception<std::runtime_error>("Cannot connect to database " + opt_db_name);
    }
}

void mysql_session::close()
{
    free_result_set();

    if (conn__) {
        mysql_close(reinterpret_cast<MYSQL*>(conn__));
        conn__ = nullptr;
    }
}

void mysql_session::query(const std::string& statement)
{
    mstatement = statement;

    if (!conn__) {
        throw_exception<std::runtime_error>("MySQL connection not established!");
    }

    if (mysql_real_query(reinterpret_cast<MYSQL*>(conn__), statement.c_str(), statement.length())) {
        auto errn = mysql_errno(reinterpret_cast<MYSQL*>(conn__));
        const char* errmsg = mysql_error(reinterpret_cast<MYSQL*>(conn__));

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


void mysql_session::query(dbm::kind::prepared_statement& stmt)
{
    MYSQL_STMT* stmt_handle = mysql_stmt_init(reinterpret_cast<MYSQL*>(conn__));
    auto result = mysql_stmt_prepare(stmt_handle, stmt.statement().c_str(), stmt.statement().size());

    MYSQL_BIND bind[stmt.size()];
    my_bool isNull[stmt.size()];
    memset(bind, 0, sizeof(MYSQL_BIND) * stmt.size());

    for (auto i = 0u; i < stmt.size(); ++i)
    {
        auto& p = stmt.param(i);
        MYSQL_BIND& b = bind[i];

        b.buffer = p.buffer();

        isNull[i] = p.is_null();
        b.is_null = &isNull[i];

        switch (p.type()) {
            case dbm::kind::data_type::Nullptr:
                b.buffer_type = MYSQL_TYPE_NULL;
                break;
            case dbm::kind::data_type::Bool:
                b.buffer_type = MYSQL_TYPE_TINY;
                break;
            case dbm::kind::data_type::Int32:
                b.buffer_type = MYSQL_TYPE_LONG;
                break;
            case dbm::kind::data_type::Int16:
                b.buffer_type = MYSQL_TYPE_SHORT;
                break;
            case dbm::kind::data_type::Int64:
                b.buffer_type = MYSQL_TYPE_LONGLONG;
                break;
            case dbm::kind::data_type::UInt32:
                b.buffer_type = MYSQL_TYPE_LONG;
                b.is_unsigned = true;
                break;
            case dbm::kind::data_type::UInt16:
                b.buffer_type = MYSQL_TYPE_SHORT;
                b.is_unsigned = true;
                break;
            case dbm::kind::data_type::UInt64:
                b.buffer_type = MYSQL_TYPE_LONGLONG;
                b.is_unsigned = true;
                break;
            case dbm::kind::data_type::Double:
                b.buffer_type = MYSQL_TYPE_DOUBLE;
                break;
            case dbm::kind::data_type::String:
                b.buffer_type = MYSQL_TYPE_STRING;
                break;
            default:
                throw_exception("Type not supported"); // TODO: handle other types
        }


    }
}

kind::sql_rows mysql_session::select_rows(const std::string& statement)
{
    kind::sql_rows rows;
    unsigned num_fields = 0;

    /* free result set if it holds any data */
    free_result_set();

    /* execute query */
    query(statement);

    /* grab the result */
    res_set__ = mysql_store_result(reinterpret_cast<MYSQL*>(conn__));
    if (!res_set__) {
        if (mysql_field_count(reinterpret_cast<MYSQL*>(conn__)) > 0) {
            throw_exception<std::runtime_error>(std::string("Error processing result set : ") + mysql_error(reinterpret_cast<MYSQL*>(conn__)) + last_statement_info());
        }
        else {
            throw_exception<std::runtime_error>(std::string("Error rows affected : ") + mysql_error(reinterpret_cast<MYSQL*>(conn__)) + last_statement_info());
        }
    }

    /* field names */
    {
        mysql_field_seek(reinterpret_cast<MYSQL_RES*>(res_set__), 0);
        num_fields = mysql_num_fields(reinterpret_cast<MYSQL_RES*>(res_set__));
        kind::sql_fields fields_tmp;
        fields_tmp.reserve(num_fields);

        for (unsigned i = 0; i < num_fields; i++) {
            MYSQL_FIELD* field = mysql_fetch_field(reinterpret_cast<MYSQL_RES*>(res_set__));
            fields_tmp.push_back(field->name);
        }

        /* setup rows object field names */
        rows.set_field_names(std::move(fields_tmp));
    }

    /* fetch rows */
    MYSQL_ROW mysql_row;
    while ((mysql_row = mysql_fetch_row(reinterpret_cast<MYSQL_RES*>(res_set__))) != nullptr) {

        kind::sql_row& r = rows.emplace_back();
        r.set_fields(&rows.field_names(), &rows.field_map());
        r.reserve(num_fields);

        unsigned long* lenghts = mysql_fetch_lengths(reinterpret_cast<MYSQL_RES*>(res_set__));

        for (unsigned i = 0; i < num_fields; i++) {
            r.emplace_back(mysql_row[i], lenghts[i]);
        }
    }

    /* check error */
    if (mysql_errno(reinterpret_cast<MYSQL*>(conn__)) != 0) {
        throw_exception<std::runtime_error>("Fetch row error");
    }

    //    sql.check_errors();
    return rows;
}

std::string mysql_session::write_model_query(const model& m) const
{
    return detail::model_query_helper<detail::param_session_type_mysql>(m).write_query();
}

std::string mysql_session::read_model_query(const model& m, const std::string& extra_condition) const
{
    return detail::model_query_helper<detail::param_session_type_mysql>(m).read_query(extra_condition);
}

std::string mysql_session::delete_model_query(const model& m) const
{
    return detail::model_query_helper<detail::param_session_type_mysql>(m).delete_query();
}

std::string mysql_session::create_table_query(const model& m, bool if_not_exists) const
{
    return detail::model_query_helper<detail::param_session_type_mysql>(m).create_table_query(if_not_exists);
}

std::string mysql_session::drop_table_query(const model& m, bool if_exists) const
{
    return detail::model_query_helper<detail::param_session_type_mysql>(m).drop_table_query(if_exists);
}

void mysql_session::transaction_begin()
{
    query("BEGIN");
}

void mysql_session::transaction_commit()
{
    query("COMMIT");
}

void mysql_session::transaction_rollback()
{
    query("ROLLBACK");
}

void mysql_session::free_result_set()
{
    if (res_set__) {
        mysql_free_result(reinterpret_cast<MYSQL_RES*>(res_set__));
        res_set__ = nullptr;
    }
}

}// namespace dbm

#endif
