#ifndef DBM_POOL_HPP
#define DBM_POOL_HPP

#include "pool_intern_item.hpp"
#include "pool_connection.hpp"
#include <vector>
#include <map>
#include <numeric>
#include <shared_mutex>
#include <atomic>

namespace dbm {

class DBM_EXPORT pool
{
public:
    using MakeSessionFn = std::function<std::shared_ptr<session>()>;

    struct statistics
    {
        size_t n_conn {0};
        size_t n_active_conn {0};
        size_t n_idle_conn {0};
        size_t n_acquired {0};
        size_t n_acquiring {0};
        size_t n_acquiring_waiting {0};
        size_t n_max_conn {0};
        size_t n_timeouts {0};
        size_t n_heartbeats {0};
        std::map<long, size_t> acquire_stat;
    };


    pool();

    pool(pool const&) = delete;

    pool(pool&&) = delete;

    ~pool();

    pool& operator=(pool const&) = delete;

    pool& operator=(pool&&) = delete;

    auto max_connections() const
    {
        std::shared_lock lock(mtx_);
        return max_conn_;
    }

    void set_max_connections(size_t n)
    {
        std::lock_guard lock(mtx_);
        max_conn_ = n;
    }

    auto acquire_timeout() const
    {
        std::shared_lock lock(mtx_);
        return acquire_timeout_;
    }

    void set_acquire_timeout(std::chrono::milliseconds to)
    {
        std::lock_guard lock(mtx_);
        acquire_timeout_ = to;
    }

    void set_session_initializer(MakeSessionFn&& fn)
    {
        std::lock_guard lock(mtx_);
        make_session_ = fn;
    }

    void set_heartbeat_interval(std::chrono::milliseconds ms)
    {
        std::lock_guard lock(mtx_);
        heartbeat_interval_ = ms;
    }

    void set_heartbeat_query(std::string query)
    {
        std::lock_guard lock(mtx_);
        heartbeat_query_ = std::move(query);
    }

    void reset_stat()
    {
        std::lock_guard lock(mtx_);
        stat_ = {};
    }

    void reset_heartbeats_counter()
    {
        std::lock_guard lock(mtx_);
        heartbeat_counter_ = 0;
    }

    statistics stat() const
    {
        statistics s;
        std::shared_lock lock(mtx_); // TODO: locking mutex is not the best way to get stat data
        s = stat_;
        s.n_conn = sessions_idle_.size() + sessions_active_.size();
        s.n_active_conn = sessions_active_.size();
        s.n_idle_conn = sessions_idle_.size();
        s.n_heartbeats = heartbeat_counter_;
        return s;
    }

    pool_connection acquire();

    size_t num_connections() const;
    size_t num_active_connections() const;
    size_t num_idle_connections() const;
    size_t heartbeats_count() const { return heartbeat_counter_; }

    auto debug_log() const
    {
        return utils::debug_logger(utils::debug_logger::level::Debug);
    }

    auto error_log() const
    {
        return utils::debug_logger(utils::debug_logger::level::Error);
    }

private:
    void release(session* s);
    void notify_release();
    void heartbeat_task();
    void calculate_wait_time(pool_intern_item::clock_t::time_point tp1, pool_intern_item::clock_t::time_point tp2);

    size_t max_conn_ {10};
    MakeSessionFn make_session_;
    std::shared_mutex mutable mtx_; // mutex protecting sessions_
    std::condition_variable cv_;
    std::mutex mutable cv_mtx_; // mutex protecting cv_
    std::chrono::milliseconds acquire_timeout_ {5000};
    std::atomic<size_t> heartbeat_counter_ {0};
    std::string heartbeat_query_ {"SELECT 1" };
    std::chrono::milliseconds heartbeat_interval_ {5000};
    std::thread thr_;
    std::atomic<bool> do_run_ {false};

    std::unordered_map<::dbm::session*, std::unique_ptr<pool_intern_item>> sessions_active_;
    std::unordered_map<::dbm::session*, std::unique_ptr<pool_intern_item>> sessions_idle_;
    std::chrono::milliseconds stat_wait_step_ {100};
    statistics stat_;
};

DBM_INLINE pool::pool()
{
    do_run_ = true;
    thr_ = std::thread([this]{ heartbeat_task(); });
}


DBM_INLINE pool::~pool()
{
    debug_log() << "dbm::pool : Exit pool begin";

    do_run_ = false;

    if (thr_.joinable())
        thr_.join();

    while (true) {

        std::unique_lock lock(mtx_);
        if (sessions_active_.empty() && sessions_idle_.empty())
            break;

        debug_log() << "dbm::pool : Exit pool : waiting connections to close";

        if (!sessions_idle_.empty()) {
            sessions_idle_.clear();
        }

        lock.unlock();

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    debug_log() << "dbm::pool : Exit pool end";
}

DBM_INLINE pool_connection pool::acquire()
{
    using clock_t = pool_intern_item::clock_t;
    auto started = clock_t::now();
    auto expires = started + acquire_timeout_;
    bool first_time_in_loop = true;

    utils::execute_at_exit atexit([this]{ --stat_.n_acquiring; });

    while (true) {

        // Lock mutex
        std::unique_lock lock(mtx_);

        if (first_time_in_loop) {
            first_time_in_loop = false;
            ++stat_.n_acquiring;
            if (stat_.n_acquiring > stat_.n_acquiring_waiting)
                stat_.n_acquiring_waiting = stat_.n_acquiring;
        }

        // Check existing connections
        auto it_available = std::find_if(sessions_idle_.begin(), sessions_idle_.end(), [](auto const& it) {
            return it.second->state_ == pool_intern_item::state::idle;
        });

        if (it_available != sessions_idle_.end()) {
            // Idle connection will be reused
            auto& session = it_available->second->session_;
            debug_log() << "dbm::pool : Activating idle session " << session.get();
            it_available->second->state_ = pool_intern_item::state::active;
            sessions_active_[it_available->first] = std::move(it_available->second);
            sessions_idle_.erase(it_available);
            if (sessions_active_.size() > stat_.n_max_conn)
                stat_.n_max_conn = sessions_active_.size();
            ++stat_.n_acquired;
            calculate_wait_time(started, clock_t::now());
            return pool_connection(session, [this, ptr = session.get()]() {
                release(ptr);
            });
        }

        // All connections active or none initialized
        if (sessions_active_.size() + sessions_idle_.size() >= max_conn_) {

            // Check timeout
            if (clock_t::now() - started > acquire_timeout_) {
                stat_.n_timeouts++;
                throw_exception("Connection acquire timeout");
            }

            // Wait for a free connection with timeout
            std::unique_lock cv_lk(cv_mtx_);
            lock.unlock();
            cv_.wait_until(cv_lk, expires);
            continue;
        }

        // Add connection instance
        if (!make_session_)
            throw_exception("Session initializer function not defined");

        auto* new_intern_item = new pool_intern_item(make_session_());
        auto* ptr = new_intern_item->session_.get();
        sessions_active_[ptr] = std::unique_ptr<pool_intern_item>(new_intern_item);
        if (sessions_active_.size() > stat_.n_max_conn)
            stat_.n_max_conn = sessions_active_.size();
        ++stat_.n_acquired;
        debug_log() << "dbm::pool : Creating session " << ptr;
        calculate_wait_time(started, clock_t::now());

        // Return pool_connection
        return pool_connection(new_intern_item->session_, [this, ptr]() {
            release(ptr);
        });
    }
}

DBM_INLINE void pool::release(session* s)
{
    debug_log() << "dbm::pool : Release session " << s;

    std::unique_lock lock(mtx_);

    // TODO: maybe clear result set ?

    auto it = sessions_active_.find(s);
    if (it != sessions_active_.end()) {
        it->second->state_ = pool_intern_item::state::idle;
        it->second->heartbeat_time_ = pool_intern_item::clock_t::now();
        if (it->second->session_->is_connected()) {
            sessions_idle_[s] = std::move(it->second);
        }
        sessions_active_.erase(it);
        lock.unlock();
        notify_release();
        return;
    }

    throw_exception("No such connection to release"); // TODO: maybe just quiet exit without exception?
}

DBM_INLINE size_t pool::num_connections() const
{
    std::shared_lock lock(mtx_);
    return sessions_active_.size() + sessions_idle_.size();
}

DBM_INLINE size_t pool::num_active_connections() const
{
    std::shared_lock lock(mtx_);
    return sessions_active_.size();
}

DBM_INLINE size_t pool::num_idle_connections() const
{
    std::shared_lock lock(mtx_);
    return sessions_idle_.size();
}

DBM_INLINE void pool::notify_release()
{
//    std::lock_guard lk(cv_mtx_);
    cv_.notify_all();
}

DBM_INLINE void pool::heartbeat_task()
{
    using clock_t = pool_intern_item::clock_t;

    // initial sleep
    std::chrono::milliseconds sleep_time = std::chrono::milliseconds(500);

    while (do_run_) {

        std::this_thread::sleep_for(sleep_time);

        if (heartbeat_interval_ == std::chrono::seconds(0))
            continue;

        // try to lock mutex
        if (mtx_.try_lock()) {
            // we managed to acquire mutex lock

            // find items and perform heartbeat query if necessary
            std::vector<pool_intern_item*> items;

            for (auto& it : sessions_idle_) {
                if (it.second->state_ == pool_intern_item::state::idle && clock_t::now() - it.second->heartbeat_time_ >  heartbeat_interval_) {
                    it.second->state_ = pool_intern_item::state::pending_heartbeat;
                    items.push_back(it.second.get());
                }
            }

            // Unlock mutex and perform heartbeats
            // we don't need mutex lock as sessions on which heartbeat will be
            // performed already have state pending_heartbeat
            mtx_.unlock();

            std::vector<pool_intern_item*> items_failed;

            for (auto* it : items) {
                try {
                    debug_log() << "dbm::pool : Performing heartbeat session " << it->session_.get();
                    if (!it->session_->select(heartbeat_query_).empty()) {
                        it->heartbeat_time_ = clock_t::now();
                        it->state_ = pool_intern_item::state::idle;
                        ++heartbeat_counter_;
                    }
                }
                catch (std::exception& e) {
                    error_log() << "dbm::pool : Heartbeat error session " << it->session_.get() << " : " << e.what();
                    it->state_ = pool_intern_item::state::canceled;
                    items_failed.push_back(it);
                }
            }

            if (!items_failed.empty()) {
                // remove items from idle session list
                mtx_.lock();

                for (auto* it : items_failed) {
                    debug_log() << "dbm::pool : Remove session " << it->session_.get();
                    sessions_idle_.erase(it->session_.get());
                }

                // notify connections released
                mtx_.unlock();
                notify_release();
            }

            sleep_time = std::chrono::milliseconds(500);
        }
        else {
            // if try lock fails it is likely that pool load is high so we
            // will try later again and adjust sleep time to a lower value
            sleep_time = std::chrono::milliseconds(100);
        }
    }
}

DBM_INLINE void pool::calculate_wait_time(pool_intern_item::clock_t::time_point tp1, pool_intern_item::clock_t::time_point tp2)
{
    long dur = std::chrono::duration_cast<std::chrono::milliseconds>(tp2 - tp1).count();
    long range = dur / stat_wait_step_.count();
    long rem = dur % stat_wait_step_.count();
    range *= stat_wait_step_.count();
    if (rem)
        range += stat_wait_step_.count();
    stat_.acquire_stat[range]++;
}

}

#endif //DBM_POOL_HPP
