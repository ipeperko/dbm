#ifndef DBM_NO_DRIVER_SESSION_H
#define DBM_NO_DRIVER_SESSION_H

#include "session.hpp"

namespace dbm {

class no_driver_session : public session
{
public:
    void open() override {}
    void close() override {}

    std::string write_model_query(const model& m) const override { throw std::domain_error("write query not available from no_driver_session"); };
    std::string read_model_query(const model& m, const std::string& extra_condition="") const override { throw std::domain_error("read query not available from no_driver_session"); };
    std::string delete_model_query(const model& m) const override { throw std::domain_error("delete query not available from no_driver_session"); };
    std::string create_table_query(const model& m, bool if_not_exists) const override { throw std::domain_error("create table query not available from no_driver_session"); };
    std::string drop_table_query(const model& m, bool if_exists) const override { throw std::domain_error("drop table query not available from no_driver_session"); };
};

}

#endif //DBM_NO_DRIVER_SESSION_H
