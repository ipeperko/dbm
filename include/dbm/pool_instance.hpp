#ifndef DBM_POOL_INSTANCE_HPP
#define DBM_POOL_INSTANCE_HPP

#include "session.hpp"
#include <chrono>

namespace dbm {

class pool_instance
{
public:

    std::shared_ptr<session>& get() { return session_; }

    std::shared_ptr<session> const& get() const { return session_; }

    void set(std::shared_ptr<session> const& s) { session_ = s; }

    void set(std::shared_ptr<session>&& s) { session_ = std::move(s); }

    auto active() const { return active_; }

    void set_active(bool is_active)
    {
        if (is_active != active_) {
            active_ = is_active;
            auto dtime = std::chrono::steady_clock::now() - tp_;
            if (is_active)
                duration_idle_ += dtime;
            else
                duration_active_ += dtime;
        }
    }

private:
    bool active_ {false};
    std::shared_ptr<::dbm::session> session_;
    std::chrono::steady_clock::time_point tp_;
    std::chrono::steady_clock::duration duration_active_;
    std::chrono::steady_clock::duration duration_idle_;
};

}

#endif //DBM_POOL_INSTANCE_HPP
