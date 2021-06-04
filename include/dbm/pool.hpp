#ifndef DBM_POOL_HPP
#define DBM_POOL_HPP

#include "pool_intern_item.hpp"
#include "pool_connection.hpp"
#include <numeric>
#include <shared_mutex>

namespace dbm {

class pool
{
public:
    using MakeSessionFn = std::function<std::shared_ptr<session>()>;

    void set_max_connections(size_t n)
    {
        max_conn_ = n;
        sessions_.reserve(n);
    }

    void set_session_initializer(MakeSessionFn&& fn) { make_session_ = fn; }

    pool_connection acquire();

    size_t num_connections() const;
    size_t num_active_connections() const;
    size_t num_idle_connections() const;

private:
    void release(session* s);

    size_t max_conn_ {0};
    std::vector<pool_intern_item> sessions_;
    MakeSessionFn make_session_;
    std::shared_mutex mutable mtx_;
};

DBM_INLINE pool_connection pool::acquire()
{
    std::unique_lock lock(mtx_);

    for  (auto& it : sessions_) {
        if (!it.active()) {
            it.set_active(true);
            return pool_connection(it.get(), [this, cn=it.get().get()]() {
                    release(cn);
                   });
//            return *it.get();
        }
    }

    // all connections active or none initialized
    if (sessions_.size() >= max_conn_) {
        lock.unlock();
        // TODO: wait for free connection
        throw_exception("TODO: wait for free connection");
    }

    auto& s = sessions_.emplace_back();
    if (!make_session_)
        throw_exception("Session initializer function not defined");
    s.set(make_session_());
    s.set_active(true);
    return pool_connection(s.get(), [this, cn=s.get().get()]() {
        release(cn);
    });
}

DBM_INLINE void pool::release(session* s)
{
    std::unique_lock lock(mtx_);

    // TODO: maybe clear result set ?

    for (auto& it : sessions_) {
        if (it.get().get() == s) {
            it.set_active(false);
            return;
        }
    }

    throw_exception("No such connection to release"); // TODO: maybe just quiet exit?
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

}

#endif //DBM_POOL_HPP
