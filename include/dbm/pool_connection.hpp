#ifndef DBM_POOL_CONNECTION_HPP
#define DBM_POOL_CONNECTION_HPP

#include <dbm/session.hpp>

namespace dbm {

template<typename DBSession, typename DBSessionRessetter>
class DBM_EXPORT pool_connection
{
public:
    pool_connection() = default;

    pool_connection(std::shared_ptr<DBSession> p, DBSessionRessetter&& resetter)
        : p_(std::move(p))
        , resetter_(std::move(resetter))
    {
        if (!p_)
            throw_exception("pool connection constructed without session");
    }

    pool_connection(pool_connection const&) = delete;

    pool_connection(pool_connection&&) noexcept = default;

    ~pool_connection()
    {
        release();
    }

    pool_connection& operator=(pool_connection const&) = delete;

    pool_connection& operator=(pool_connection&&) noexcept = default;

    DBSession& get()
    {
        if (!p_)
            throw_exception("pool_connection::get session is null");
        return *p_;
    }

    DBSession const& get() const
    {
        if (!p_)
            throw_exception("pool_connection::get session is null");
        return *p_;
    }

    DBSession& operator*()
    {
        return get();
    }

    DBSession const& operator*() const
    {
        return get();
    }

    bool is_valid() const
    {
        return p_ != nullptr;
    }

    operator bool() const
    {
        return p_ != nullptr;
    }

    // releases session from pool
    void release()
    {
        resetter_();
        p_ = nullptr;
    }

private:
    std::shared_ptr<DBSession> p_;
    DBSessionRessetter resetter_;
};

}

#endif //DBM_POOL_CONNECTION_HPP
