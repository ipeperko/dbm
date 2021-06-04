#ifndef DBM_POOL_INTERN_ITEM_HPP
#define DBM_POOL_INTERN_ITEM_HPP

#include "session.hpp"
#include <chrono>

namespace dbm {

class pool_intern_item
{
public:

    enum class state
    {
        idle,
        pending_heartbeat,
        active,
        close_requested
    };

    struct shared_data
    {
        std::chrono::milliseconds heartbeat_interval;
        std::string heartbeat_query;
        std::function<void(pool_intern_item*)> notify_available;
        std::function<void(pool_intern_item*)> notify_heartbeat_error;
        std::function<void(pool_intern_item*)> heartbeat_completed;
    };

    explicit pool_intern_item(std::shared_ptr<shared_data> shared)
        : shared_(std::move(shared))
    {
        state_ = state::active;
    }

    ~pool_intern_item()
    {
        timer_.cancel();

        if (thr_.joinable())
            thr_.join();
    }

    std::shared_ptr<session>& get() { return session_; }

    std::shared_ptr<session> const& get() const { return session_; }

    void set(std::shared_ptr<session> const& s) { session_ = s; }

    void set(std::shared_ptr<session>&& s) { session_ = std::move(s); }

    bool active() const
    {
        std::lock_guard lock(mtx_);
        return state_ == state::active;
    }

    bool activate()
    {
        std::lock_guard lock(mtx_);
        if (state_ == state::idle) {
            timer_.cancel();
//            if (thr_.joinable())
//                thr_.join();
            state_ = state::active;
            return true;
        }
        return false;
    }

    void deactivate()
    {
        std::lock_guard lock(mtx_);
        timer_.cancel();
//        if (thr_.joinable())
//            thr_.join();
        state_ = state::idle;
//        start_heartbeat_timer();
    }

private:

    // Note: caller must acquire mutex lock!
    void start_heartbeat_timer()
    {
        if (shared_->heartbeat_interval.count() > 0) {
            timer_.expires_after(shared_->heartbeat_interval, nullptr);

            if (thr_.joinable())
                thr_.join();

            thr_ = std::thread([this]{
                while (!timer_.canceled()) {
                    timer_.wait();

                    if (timer_.canceled())
                        return;

                    on_timer_timeout();

                    if (timer_.canceled())
                        break;

                    timer_.expires_after(shared_->heartbeat_interval, nullptr);
              }
            });
        }
    }

    void on_timer_timeout()
    {
        std::unique_lock lock(mtx_);

        state_ = state::pending_heartbeat;

        try {
            if (session_->select(shared_->heartbeat_query).empty())
                throw std::runtime_error("heartbeat result empty");

            state_ = state::idle;

            lock.unlock();

            if (shared_->heartbeat_completed)
                shared_->heartbeat_completed(this);

            if (shared_->notify_available)
                shared_->notify_available(this);
        }
        catch (std::exception& e) {
            timer_.cancel();
            state_ = state::close_requested;

            if (shared_->notify_heartbeat_error)
                shared_->notify_heartbeat_error(this);
        }
    }

    std::mutex mutable mtx_;
    state state_ {state::idle};
    std::shared_ptr<::dbm::session> session_;
    utils::timer<std::chrono::steady_clock> timer_;
    std::shared_ptr<shared_data> shared_;
    std::thread thr_;
//    std::chrono::steady_clock::time_point tp_;
//    std::chrono::steady_clock::duration duration_active_;
//    std::chrono::steady_clock::duration duration_idle_;
};

}

#endif //DBM_POOL_INTERN_ITEM_HPP
