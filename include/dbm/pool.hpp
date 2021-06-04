#ifndef DBM_POOL_HPP
#define DBM_POOL_HPP

#include "pool_intern_item.hpp"
#include "pool_connection.hpp"
#include <list>
#include <numeric>
#include <shared_mutex>
#include <atomic>

namespace dbm {

class pool
{
public:
    using MakeSessionFn = std::function<std::shared_ptr<session>()>;

    pool()
        : items_shared_(std::make_shared<pool_intern_item::shared_data>())
    {
        items_shared_->heartbeat_query = "SELECT 1";
        items_shared_->heartbeat_interval = std::chrono::seconds(5);
        items_shared_->notify_available = [this](auto*) {
            notify_release();
        };
        items_shared_->notify_heartbeat_error = [this](auto* item) {
            std::cout << "items_shared_->notify_heartbeat_error" << std::endl;
            // We need to remove from different thread to avoid deadlock
            std::thread([this, item]() {
                remove(item);
            }).detach();
        };
        items_shared_->heartbeat_completed = nullptr; // disabled by default
    }

    auto max_connections() const { return max_conn_; }

    void set_max_connections(size_t n)
    {
        max_conn_ = n;
    }

    auto acquire_timeout() const { return acquire_timeout_; }

    void set_acquire_timeout(std::chrono::milliseconds to) { acquire_timeout_ = to; }

    void set_session_initializer(MakeSessionFn&& fn) { make_session_ = fn; }

    void set_heartbeat_interval(std::chrono::milliseconds ms) { items_shared_->heartbeat_interval = ms; }

    void set_heartbeat_query(std::string query) { items_shared_->heartbeat_query = std::move(query); }

    void enable_heartbeat_count(bool enable)
    {
        if (enable) {
            items_shared_->heartbeat_completed = [this](auto*) {
                ++heartbeat_counter_;
            };
        }
        else {
            items_shared_->heartbeat_completed = nullptr;
            heartbeat_counter_ = 0;
        }
    }

    pool_connection acquire();

    size_t num_connections() const;
    size_t num_active_connections() const;
    size_t num_idle_connections() const;
    size_t heartbeats_count() const { return heartbeat_counter_; }

private:
    void remove(pool_intern_item*);
    void release(session* s);
    void notify_release();

    size_t max_conn_ {10};
    std::list<pool_intern_item> sessions_;
    MakeSessionFn make_session_;
    std::shared_mutex mutable mtx_; // mutex protecting sessions_
    std::condition_variable cv_;
    std::mutex mutable cv_mtx_; // mutex protecting cv_
    std::chrono::milliseconds acquire_timeout_ {5000};
    std::shared_ptr<pool_intern_item::shared_data> items_shared_;
    std::atomic<size_t> heartbeat_counter_ {0};
};

DBM_INLINE pool_connection pool::acquire()
{
    using clock_t = std::chrono::steady_clock;
    auto tp0 = clock_t::now();
    auto expire = tp0 + acquire_timeout_;

    while (true) {
        // Lock mutex
        std::unique_lock lock(mtx_);

        // Check existing connections
        for (auto& it : sessions_) {
            if (it.activate()) {
                return pool_connection(it.get(), [this, cn = it.get().get()]() {
                    release(cn);
                });
            }
        }

        // All connections active or none initialized
        if (sessions_.size() >= max_conn_) {
            // Inlock
            lock.unlock();

            // Check timeout
            std::chrono::duration<double, std::milli> dt = clock_t::now() - tp0;
            if (dt > acquire_timeout_) {
                //std::string msg = "connection timeout dt = " + std::to_string(dt.count()) + " > acquire_timeout_ = " + std::to_string(acquire_timeout_.count());
                //throw_exception(msg);
                throw_exception("connection acquire timeout");
            }

            // Wait for free connection with timeout
            std::unique_lock cv_lk(cv_mtx_);
            cv_.wait_until(cv_lk, expire);

            // continue one more time and try to acquire connection
            //std::this_thread::yield();
            continue;
        }

        // Add connection instance
        auto& s = sessions_.emplace_back(items_shared_);

        // Initialize instance
        if (!make_session_)
            throw_exception("Session initializer function not defined");
        s.set(make_session_());

        // Return pool_connection
        return pool_connection(s.get(), [this, cn = s.get().get()]() {
            release(cn);
        });
    }
}

DBM_INLINE void pool::remove(pool_intern_item* item)
{
    std::unique_lock lock(mtx_);

    std::cout << "remove item " << item << " session " << item->get().get() << std::endl;

    auto it = std::find_if(sessions_.begin(), sessions_.end(), [&](auto const& it) {
        return &it == item;
    });

    if (it != sessions_.end()) {
        sessions_.erase(it);
        lock.unlock();
        notify_release();
    }
}

DBM_INLINE void pool::release(session* s)
{
    std::unique_lock lock(mtx_);

    std::cout << "release session " << s << std::endl;

    // TODO: maybe clear result set ?

    for (auto& it : sessions_) {
        if (it.get().get() == s) {
            it.deactivate();
            lock.unlock(); // TODO: should unlock mtx_ here?
            notify_release();
            return;
        }
    }

    throw_exception("No such connection to release"); // TODO: maybe just quiet exit without exception?
}

DBM_INLINE size_t pool::num_connections() const
{
    std::shared_lock lock(mtx_);
    return sessions_.size();
}

DBM_INLINE size_t pool::num_active_connections() const
{
    std::shared_lock lock(mtx_);
    return std::accumulate(sessions_.begin(), sessions_.end(), 0, [](size_t n, pool_intern_item const& s) {
        return n + (s.active() ? 1 : 0);
    });
}

DBM_INLINE size_t pool::num_idle_connections() const
{
    std::shared_lock lock(mtx_);
    return std::accumulate(sessions_.begin(), sessions_.end(), 0, [](size_t n, pool_intern_item const& s) {
        return n + (s.active() ? 0 : 1);
    });
}

DBM_INLINE void pool::notify_release()
{
    std::lock_guard lk(cv_mtx_);
    cv_.notify_all();
}

}

#endif //DBM_POOL_HPP
