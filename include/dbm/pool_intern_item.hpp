#ifndef DBM_POOL_INTERN_ITEM_HPP
#define DBM_POOL_INTERN_ITEM_HPP

#include "session.hpp"
#include <chrono>

namespace dbm {

class DBM_EXPORT pool_intern_item
{
public:
    using clock_t = std::chrono::steady_clock;

    enum class state
    {
        idle,
        pending_heartbeat,
        active,
        canceled
    };

    explicit pool_intern_item(std::shared_ptr<::dbm::session>&& s)
        : session_(std::move(s))
    {
        state_ = state::active;
    }

    state state_ {state::idle};
    std::shared_ptr<::dbm::session> session_;
    clock_t::time_point heartbeat_time_;

//    std::chrono::steady_clock::duration duration_active_;
//    std::chrono::steady_clock::duration duration_idle_;
};

}

#endif //DBM_POOL_INTERN_ITEM_HPP