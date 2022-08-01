#ifndef DBM_POOL_CONNECTION_HPP
#define DBM_POOL_CONNECTION_HPP

#include <dbm/session.hpp>

namespace dbm {

template<typename PoolType>
class DBM_EXPORT pool_connection
{
public:
    using pool_type = PoolType;
    using db_session_type = typename PoolType::db_session_type;

    pool_connection() = delete;

    pool_connection(pool_type& pool, std::shared_ptr<db_session_type> p)
        : pool_(pool)
        , session_(std::move(p))
    {
    }

    pool_connection(pool_connection const&) = delete;

    pool_connection(pool_connection&& oth) noexcept
        : pool_(oth.pool_)
        , session_(std::move(oth.session_))
    {
    }

    ~pool_connection()
    {
        release();
    }

    pool_connection& operator=(pool_connection const&) = delete;

    pool_connection& operator=(pool_connection&& oth) noexcept
    {
        if (this != &oth) {
            // release this first
            try {
                release();
            }
            catch(std::exception&) {}

            if (&pool_ == &oth.pool_) {
                // move is only possible if not the same instance and connections belong to the same pool
                session_ = std::move(oth.session_);
            }
            else {
                // reset the oth connection
                try {
                    oth.release();
                }
                catch(std::exception&) {}
            }
        }
        return *this;
    }

    db_session_type& get()
    {
        if (!session_)
            throw_exception("pool_connection::get session is null");
        return *session_;
    }

    db_session_type const& get() const
    {
        if (!session_)
            throw_exception("pool_connection::get session is null");
        return *session_;
    }

    db_session_type& operator*()
    {
        return get();
    }

    db_session_type const& operator*() const
    {
        return get();
    }

    bool is_valid() const
    {
        return session_ != nullptr;
    }

    operator bool() const
    {
        return session_ != nullptr;
    }

    // releases db session
    void release()
    {
        if (session_) {
            pool_.release(session_.get());
            session_ = nullptr;
        }
    }

private:
    pool_type& pool_;
    std::shared_ptr<db_session_type> session_;
};

} // namespace dbm

#endif //DBM_POOL_CONNECTION_HPP
