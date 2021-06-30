#ifndef DBM_POOL_CONNECTION_HPP
#define DBM_POOL_CONNECTION_HPP

#include <dbm/session.hpp>

namespace dbm {

class DBM_EXPORT pool_connection
{
public:
    pool_connection() = default;

    pool_connection(std::shared_ptr<session> p, std::function<void(void)>&& resetter)
        : p_(std::move(p))
        , resetter_(std::move(resetter))
    {
        if (!p_)
            throw_exception("pool connection constructed without session");

        if (!resetter_)
            throw_exception("pool_connection constructed without resetter");
    }

    pool_connection(pool_connection const&) = delete;

    pool_connection(pool_connection&&) = default;

    ~pool_connection()
    {
        release();
    }

    pool_connection& operator=(pool_connection const&) = delete;

    pool_connection& operator=(pool_connection&&) = default;

    session& get()
    {
        if (!p_)
            throw_exception("pool_connection::get session is null");
        return *p_;
    }

    bool is_valid() const
    {
        return p_ != nullptr;
    }

    // releases session from pool
    void release()
    {
        if (resetter_) {
            resetter_();
            resetter_ = nullptr;
        }
        p_ = nullptr;
    }

private:
    std::shared_ptr<session> p_;
    std::function<void(void)> resetter_;
};

}

#endif //DBM_POOL_CONNECTION_HPP
