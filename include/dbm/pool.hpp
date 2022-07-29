#ifndef DBM_POOL_HPP
#define DBM_POOL_HPP

#include "pool_intern_item.hpp"
#include "pool_connection.hpp"
#include <vector>
#include <map>
#include <queue>
#include <numeric>
#include <shared_mutex>
#include <atomic>

namespace dbm {

enum class pool_event
{
    acquired,
    handover,
    timeout,
    heartbeat_success,
    heartbeat_fail,
};

DBM_INLINE std::string to_string(pool_event event)
{
    switch (event) {
        case pool_event::acquired:          return "acquired";
        case pool_event::handover:          return "handover";
        case pool_event::timeout:           return "timeout";
        case pool_event::heartbeat_success: return "heartbeat_success";
        case pool_event::heartbeat_fail:    return "heartbeat_fail";
        default:                            return "unknown";
    }
}

template<typename DBSession, typename SessionInitializer>
class DBM_EXPORT pool
{
    friend class pool_connection<pool>;
public:

    using id_t = size_t;
    using EventCallback = std::function<void(pool_event, id_t)>;
    using db_session_type = DBSession;
    using db_session_initializer_type = SessionInitializer;
    using pool_intern_item_type = pool_intern_item<db_session_type>;
    using pool_connection_type = pool_connection<pool>;

    struct statistics
    {
        size_t n_conn {0};
        size_t n_active_conn {0};
        size_t n_idle_conn {0};
        size_t n_acquired {0};
        size_t n_acquiring {0};
        size_t n_acquiring_max {0};
        size_t n_max_conn {0};
        size_t n_timeouts {0};
        size_t n_heartbeats {0};
        std::map<long, size_t> acquire_stat;
    };


    explicit pool(SessionInitializer&& session_init = SessionInitializer())
        : make_session_(std::move(session_init))
    {
        do_run_ = true;
        thr_ = std::thread([this]{ heartbeat_task(); });
    }

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

    void set_event_callback(EventCallback&& cb, bool async = true)
    {
        std::lock_guard lock(mtx_);
        event_cb_ = std::move(cb);
        event_cb_async_ = async;
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

    void reset_heartbeats_counter()
    {
        std::lock_guard lock(mtx_);
        heartbeat_counter_ = 0;
    }

    void reset_stat()
    {
        std::lock_guard lock(mtx_);
        stat_ = {};
    }

    statistics stat() const
    {
        statistics s;
        std::shared_lock lock(mtx_); // TODO: locking mutex is not the best way to get stat data
        s = stat_;
        s.n_conn = sessions_idle_.size() + sessions_active_.size();
        s.n_active_conn = sessions_active_.size();
        s.n_idle_conn = sessions_idle_.size();
        s.n_acquiring = acquiring_.size();
        s.n_heartbeats = heartbeat_counter_;
        return s;
    }

    pool_connection_type acquire();

    size_t num_connections() const
    {
        std::shared_lock lock(mtx_);
        return sessions_active_.size() + sessions_idle_.size();
    }

    size_t num_active_connections() const
    {
        std::shared_lock lock(mtx_);
        return sessions_active_.size();
    }

    size_t num_idle_connections() const
    {
        std::shared_lock lock(mtx_);
        return sessions_idle_.size();
    }

    size_t heartbeats_count() const
    {
        return heartbeat_counter_;
    }

    utils::debug_logger debug_log() const
    {
        utils::debug_logger l(utils::debug_logger::level::Debug);
        l << "dbm::pool : ";
        return l;
    }

    utils::debug_logger error_log() const
    {
        utils::debug_logger l(utils::debug_logger::level::Error);
        l << "dbm::pool : ";
        return l;
    }

    static constexpr id_t invalid_id = -1;

private:
    pool_connection_type make_pool_connection_instance(std::shared_ptr<DBSession> const& session, id_t acquire_id, bool remove, typename pool_intern_item_type::clock_t::time_point started);
    void release(DBSession* s);
    void heartbeat_task();
    void calculate_wait_time(typename pool_intern_item_type::clock_t::time_point tp1, typename pool_intern_item_type::clock_t::time_point tp2);
    void remove_acquiring(id_t id);
    void send_event(pool_event event, id_t id);

    // Session
    std::shared_mutex mutable mtx_;         // main control mutex
    std::deque<id_t> acquiring_;
    std::unordered_map<DBSession*, std::unique_ptr<pool_intern_item_type>> sessions_active_;
    std::unordered_map<DBSession*, std::unique_ptr<pool_intern_item_type>> sessions_idle_;

    // Handover
    id_t ho_id_ {0};                        // handover id
    std::shared_ptr<DBSession> ho_session_;   // handover session
    std::condition_variable cv_ho_begin_;
    std::mutex mutable mtx_cv_ho_begin_;    // mutex protecting cv_ho_begin_
    std::condition_variable cv_ho_end_;
    std::mutex mutable mtx_cv_ho_end_;      // mutex protecting cv_ho_end_

    // Heartbeat
    std::thread thr_;
    std::atomic<bool> do_run_ {false};
    std::atomic<size_t> heartbeat_counter_ {0};
    std::string heartbeat_query_ {"SELECT 1" };
    std::chrono::milliseconds heartbeat_interval_ {5000};

    SessionInitializer make_session_;
    EventCallback event_cb_;
    bool event_cb_async_ {true};
    size_t max_conn_ {10};
    std::chrono::milliseconds acquire_timeout_ {5000};
    std::chrono::milliseconds stat_wait_step_ {100};
    statistics stat_;
};

template<typename DBSession, typename SessionInitializer>
pool<DBSession, SessionInitializer>::~pool()
{
    using namespace std::chrono_literals;

    debug_log() << "Exit pool begin";

    do_run_ = false;

    if (thr_.joinable())
        thr_.join();

    while (true) {

        std::unique_lock lock(mtx_);
        if (sessions_active_.empty() && sessions_idle_.empty())
            break;

        debug_log() << "Exit pool : waiting connections to close";

        if (!sessions_idle_.empty()) {
            sessions_idle_.clear();
        }

        lock.unlock();

        std::this_thread::sleep_for(1s);
    }

    debug_log() << "Exit pool end";
}

template<typename DBSession, typename SessionInitializer>
typename pool<DBSession, SessionInitializer>::pool_connection_type
pool<DBSession, SessionInitializer>::acquire()
{
    using clock_t = typename pool_intern_item_type::clock_t;
    auto started = clock_t::now();
    auto expires = started + acquire_timeout_;
    id_t acquire_id;

    // Lock mutex
    std::unique_lock lock(mtx_);

    // Acquire id
    static id_t sid = invalid_id;
    if (sid == invalid_id)
        sid++;
    acquire_id = sid++;
    acquiring_.push_back(acquire_id);
    if (acquiring_.size() > stat_.n_acquiring_max)
        stat_.n_acquiring_max = acquiring_.size();

    debug_log() << "acquiring #" << acquire_id << " total acquiring : " << acquiring_.size();

    // Check existing connections
    auto it_available = std::find_if(sessions_idle_.begin(), sessions_idle_.end(), [](auto const& it) {
        return it.second->state_ == pool_intern_item_type::state::idle;
    });

    // If an idle session is available it will be reused
    if (it_available != sessions_idle_.end()) {
        auto* s = it_available->second->session_.get();
        debug_log() << "Activating idle session #" << acquire_id << " (" << s << ")";

        // change state
        it_available->second->state_ = pool_intern_item_type::state::active;

        // move to active map
        sessions_active_[s] = std::move(it_available->second);

        // remove from idle map
        sessions_idle_.erase(it_available);

        // return connection object
        return make_pool_connection_instance(sessions_active_[s]->session_, acquire_id, true, started);
    }

    // If all connections are active we need to wait for one
    if (sessions_active_.size() + sessions_idle_.size() >= max_conn_) {

        // Handover session pointer
        std::shared_ptr<DBSession> sh;

        // Lock handover begin condition variable
        std::unique_lock cv_lk(mtx_cv_ho_begin_);

        // Unlock main mutex
        lock.unlock();

        // Wait for a free connection with timeout
        bool r = cv_ho_begin_.wait_until(cv_lk, expires, [this, acquire_id, &sh] {
            if (ho_session_ && acquire_id == ho_id_) {
                //debug_log() << "check #" << acquire_id << " ho_id_ " << ho_id_ << " true";
                sh = std::move(ho_session_);
                return true;
            }
            else {
                //debug_log() << "check #" << acquire_id << " ho_id_ " << ho_id_ << " false";
                return false;
            }
        });

        if (r) {
            // Session acquired successfully
            debug_log() << "handover session #" << acquire_id << " (" << sh.get() << ")";

            // Notify releaser
            std::lock_guard lock_ho(mtx_cv_ho_end_);
            cv_ho_end_.notify_all();
            //debug_log() << "handover session #" << acquire_id << " notified (" << sh.get() << ")";

            // Return connection object
            return make_pool_connection_instance(sh, acquire_id, false, started);
        }
        else {
            // Handle acquire timeout

            // Remove acquire id from queue and throw exception
            lock.lock();
            if (sessions_active_.size() + sessions_idle_.size() >= max_conn_) {
                stat_.n_timeouts++;
                remove_acquiring(acquire_id);

                send_event(pool_event::timeout, acquire_id);
                throw_exception("Connection acquire timeout");
            }
            else {
                // In case there are free connection available could be that heartbeat query failed
                // and session was deleted from session_active_
                goto Make_new_session;
            }
        }
    }

    Make_new_session:

    // Create session
    auto* new_intern_item = new pool_intern_item(make_session_());
    auto* ptr = new_intern_item->session_.get();
    sessions_active_[ptr] = std::unique_ptr<pool_intern_item_type>(new_intern_item);
    debug_log() << "Creating session #" << acquire_id << " (" << ptr << ")";

    // Return connection object
    return make_pool_connection_instance(new_intern_item->session_, acquire_id, true, started);
}

template<typename DBSession, typename SessionInitializer>
typename pool<DBSession, SessionInitializer>::pool_connection_type
pool<DBSession, SessionInitializer>::make_pool_connection_instance(std::shared_ptr<DBSession> const& session, id_t acquire_id, bool remove, typename pool_intern_item_type::clock_t::time_point started)
{
    if (sessions_active_.size() > stat_.n_max_conn)
        stat_.n_max_conn = sessions_active_.size();
    ++stat_.n_acquired;
    calculate_wait_time(started, pool_intern_item_type::clock_t::now());
    if (remove)
        remove_acquiring(acquire_id);

    send_event(pool_event::acquired, acquire_id);

    return { *this, session };
}

template<typename DBSession, typename SessionInitializer>
void pool<DBSession, SessionInitializer>::release(DBSession* s)
{
    std::unique_lock lck(mtx_, std::defer_lock);
    std::unique_lock lck_ho_begin(mtx_cv_ho_begin_, std::defer_lock);
    std::lock(lck, lck_ho_begin);

    debug_log() << "Release session begin (" << s << ")";

    auto it = sessions_active_.find(s);
    if (it != sessions_active_.end()) {
        it->second->heartbeat_time_ = pool_intern_item_type::clock_t::now();

        if (!acquiring_.empty() && it->second->session_->is_connected()) {
            // Begin handover
            auto id = acquiring_.front();
            ho_id_ = id;
            ho_session_ = it->second->session_;
            acquiring_.pop_front();
            debug_log() << "session handover to #" << id << " (" << s << ")";

            // Lock handover end condition variable
            std::unique_lock lck_ho_end(mtx_cv_ho_end_);
            cv_ho_begin_.notify_all();
            lck_ho_begin.unlock();

            //debug_log() << "session handover to #" << id << " wait begin (" << s << ")";
            send_event(pool_event::handover, id);

            // Wait acquiring thread to finish handover and block another releases to prevent modifying ho_session while acquiring tasks waiting in queue
            cv_ho_end_.wait(lck_ho_end);
            //debug_log() << "session handover to #" << id << " wait finished (" << s << ")";
        }
        else {
            // No waiting tasks - move connection to idle map
            it->second->state_ = pool_intern_item_type::state::idle;
            if (it->second->session_->is_connected()) {
                sessions_idle_[s] = std::move(it->second);
            }
            sessions_active_.erase(it);
            debug_log() << "session released (" << s << ") - total active : " << sessions_active_.size() << " total idle : " << sessions_idle_.size();
        }
    }
    else {
        throw_exception("No such connection to release"); // TODO: maybe just quiet exit without exception?
    }
}

template<typename DBSession, typename SessionInitializer>
void pool<DBSession, SessionInitializer>::heartbeat_task()
{
    using namespace std::chrono_literals;

    using clock_t = typename pool_intern_item_type::clock_t;

    // initial sleep
    auto sleep_time = 500ms;

    while (do_run_) {

        std::this_thread::sleep_for(sleep_time);

        if (heartbeat_interval_ == 0s)
            continue;

        // try to lock mutex
        if (mtx_.try_lock()) {
            // we managed to acquire mutex lock

            // find items and perform heartbeat query if necessary
            std::vector<pool_intern_item_type*> items;

            auto heartbeat_candidate = [this]() {
                for (auto it = sessions_idle_.begin(); it != sessions_idle_.end(); ++it) {
                    if (it->second->state_ == pool_intern_item_type::state::idle && clock_t::now() - it->second->heartbeat_time_ > heartbeat_interval_) {
                        return it;
                    }
                }
                return sessions_idle_.end();
            };

            typename decltype(sessions_idle_)::iterator iter;

            while ((iter = heartbeat_candidate()) != sessions_idle_.end()) {
                auto* s = iter->first;
                iter->second->state_ = pool_intern_item_type::state::pending_heartbeat;
                sessions_active_[s] = std::move(iter->second);
                sessions_idle_.erase(iter);
                items.push_back(sessions_active_[s].get());

            }

            // Unlock mutex and perform heartbeats
            // we don't need mutex lock as sessions on which heartbeat will be
            // performed already have state pending_heartbeat
            mtx_.unlock();

            std::vector<pool_intern_item_type*> items_failed;
            items_failed.reserve(items.size());
            std::vector<pool_intern_item_type*> items_success;
            items_success.reserve(items.size());

            for (auto* it : items) {
                try {
                    debug_log() << "Performing heartbeat session " << it->session_.get();
                    // TODO: custom executor instead of select statement
                    if (!it->session_->select(heartbeat_query_).empty()) {
                        it->heartbeat_time_ = clock_t::now();
                        ++heartbeat_counter_;
                        items_success.push_back(it);
                    }
                    send_event(pool_event::heartbeat_success, invalid_id);
                }
                catch (std::exception& e) {
                    send_event(pool_event::heartbeat_fail, invalid_id);
                    error_log() << "Heartbeat error session " << it->session_.get() << " : " << e.what();
                    it->state_ = pool_intern_item_type::state::canceled;
                    items_failed.push_back(it);
                }
            }

            if (!items_failed.empty()) {
                // remove items from idle session list
                std::lock_guard lock_erasing(mtx_);

                for (auto* it : items_failed) {
                    debug_log() << "Remove session " << it->session_.get();
                    sessions_active_.erase(it->session_.get());
                }
            }

            for (auto* it : items_success) {
                release(it->session_.get());
            }

            sleep_time = 500ms;
        }
        else {
            // if try lock fails it is likely that pool load is high so we
            // will try later again and adjust sleep time to a lower value
            sleep_time = 100ms;
        }
    }
}

template<typename DBSession, typename SessionInitializer>
void pool<DBSession, SessionInitializer>::calculate_wait_time(typename pool_intern_item_type::clock_t::time_point tp1, typename pool_intern_item_type::clock_t::time_point tp2)
{
    long dur = std::chrono::duration_cast<std::chrono::milliseconds>(tp2 - tp1).count();
    long range = dur / stat_wait_step_.count();
    long rem = dur % stat_wait_step_.count();
    range *= stat_wait_step_.count();
    if (rem)
        range += stat_wait_step_.count();
    if (range == 0)
        range = 1;
    stat_.acquire_stat[range]++;
}

template<typename DBSession, typename SessionInitializer>
void pool<DBSession, SessionInitializer>::remove_acquiring(id_t id)
{
    for (auto it = acquiring_.begin(); it != acquiring_.end(); ++it) {
        if (*it == id) {
            acquiring_.erase(it);
            break;
        }
    }
}

template<typename DBSession, typename SessionInitializer>
void pool<DBSession, SessionInitializer>::send_event(pool_event event, id_t id)
{
    if (event_cb_) {
        if (event_cb_async_) {
            std::thread([event, id, cb = event_cb_] {
                cb(event, id);
            }).detach();
        }
        else {
            event_cb_(event, id);
        }
    }
}

} // namespace dbm

#endif //DBM_POOL_HPP
