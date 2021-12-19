#ifdef DBM_POSTGRES

#include <dbm/drivers/postgres/postgres_session.hpp>
#include <dbm/dbm_common.hpp>
#include <dbm/model_item.hpp>
#include <dbm/model.hpp>
#include <dbm/impl/model_item.ipp>
#include <dbm/impl/model.ipp>
#include <dbm/impl/session.ipp>
#include <dbm/detail/model_query_helper.hpp>

#include <postgresql/libpq-fe.h>

#define PQ_CONNECTION_HANDLE (reinterpret_cast<PGconn*>(conn_))


namespace dbm {

postgres_session::postgres_session()
{
}

postgres_session::~postgres_session()
{

}

std::unique_ptr<session> postgres_session::clone() const
{
    // TODO: clone
    return nullptr;
}

void postgres_session::init(const std::string& host_name, const std::string& user, const std::string& password, unsigned int port, const std::string& db_name)
{
    if (conn_) {
        return;
    }

    conn_ = PQsetdbLogin(host_name.c_str(),
                         std::to_string(port).c_str(),
                         "",
                         nullptr,
                         db_name.c_str(),
                         user.c_str(),
                         password.c_str()
                         );

    if (conn_ && PQstatus(PQ_CONNECTION_HANDLE) == CONNECTION_BAD) {
        std::string msg = std::string("Connection to database failed: ") +
                PQerrorMessage(PQ_CONNECTION_HANDLE);
        PQfinish(PQ_CONNECTION_HANDLE);
        conn_ = nullptr;
        throw_exception<std::runtime_error>(msg);
    }
}

void postgres_session::open()
{

}
void postgres_session::close()
{
    if (conn_)
        PQfinish(PQ_CONNECTION_HANDLE);

    conn_ = nullptr;
}

void postgres_session::query(const std::string& statement)
{
    PGresult *res = PQexec(PQ_CONNECTION_HANDLE, statement.c_str());

    dbm::utils::execute_at_exit([res] {
        PQclear(res);
    });

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::string msg = std::string("Postgres error : ") + PQerrorMessage(PQ_CONNECTION_HANDLE);
        throw_exception(msg);
    }
}

void postgres_session::init_prepared_statement(kind::prepared_statement& stmt)
{

}

void postgres_session::query(kind::prepared_statement& stmt)
{

}

std::vector<std::vector<container_ptr>> postgres_session::select(kind::prepared_statement& stmt)
{
    return {};
}

std::string postgres_session::write_model_query(const model& m) const
{
    return {};
}

std::string postgres_session::read_model_query(const model& m, const std::string& extra_condition) const
{
    return {};
}

std::string postgres_session::delete_model_query(const model& m) const
{
    return {};
}

std::string postgres_session::create_table_query(const model& m, bool if_not_exists) const
{
    return detail::model_query_helper<detail::param_session_type_postgres>(m).create_table_query(if_not_exists);
}

std::string postgres_session::drop_table_query(const model& m, bool if_exist) const
{
    return {};
}

void postgres_session::transaction_begin()
{

}

void postgres_session::transaction_commit()
{

}

void postgres_session::transaction_rollback()
{

}

kind::sql_rows postgres_session::select_rows(const std::string& statement)
{
    return {};
}

} // namespace dbm

#endif