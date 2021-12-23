#ifdef DBM_MYSQL

#define BOOST_TEST_MODULE dbm_manual

#include "dbm/dbm.hpp"
#include "../db_settings.h"
#include "../common.h"
#ifdef DBM_MYSQL
#include <dbm/drivers/mysql/mysql_session.hpp>
#endif

using namespace boost::unit_test;

BOOST_AUTO_TEST_SUITE(TstManual)

BOOST_AUTO_TEST_CASE(test1)
{
    dbm::mysql_session session;
    db_settings::instance().init_mysql_session(session, db_settings::instance().test_db_name);

    BOOST_TEST(session.is_connected());
    BOOST_TEST(session.select("SELECT 1").size() == 1);

    printf("Stop MySQL server and press any key to continue or 'c' to cancel");
    int c = getchar();
    if (c == 'c' || c == 'C')
        return;

    BOOST_REQUIRE_THROW(session.select("SELECT 1"), std::exception);
    BOOST_TEST(!session.is_connected());
}

BOOST_AUTO_TEST_SUITE_END()

#endif //#ifdef DBM_MYSQL