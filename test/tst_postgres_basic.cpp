#ifdef DBM_POSTGRES

#include "dbm/dbm.hpp"
#include "db_settings.h"
#include "common.h"
#include <dbm/drivers/postgres/postgres_session.hpp>

#include <postgresql/libpq-fe.h>

using namespace boost::unit_test;
using namespace std::chrono_literals;

BOOST_AUTO_TEST_SUITE(TstPostgres)

BOOST_AUTO_TEST_CASE(postgres_init)
{
    auto db = db_settings::instance().get_postgres_session();

    auto native_handle = db->native_handle();
    auto conn = reinterpret_cast<PGconn*>(native_handle);
    int ver = PQserverVersion(conn);

    std::cout << "Server version: " << ver;

//    PQfinish(conn);

    struct Data {
        std::string name;
    } data;

    dbm::model model("basic_test",
                     {
                         { dbm::key("id"), dbm::local<int>(), dbm::primary(true), dbm::not_null(true) },
                         { dbm::key("name"), dbm::binding(data.name)}

                       });

    model.create_table(*db);
}

BOOST_AUTO_TEST_SUITE_END()

#endif