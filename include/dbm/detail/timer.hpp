#ifndef DBM_TIMER_HPP
#define DBM_TIMER_HPP

#include <chrono>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace dbm::utils {

template<typename Clock = std::chrono::steady_clock>
class timer
{
public:
    using time_point_t = std::chrono::time_point<Clock>;
    using callback_t = std::function<void(void)>;

    timer() = default;

    ~timer()
    {
        cancel();
        if (thr_.joinable()) {
            thr_.join();
        }
    }

    void expires_at(time_point_t tp, callback_t&& cb)
    {
        expiry_ = tp;
        start_thread(std::move(cb));
    }

    template<typename Duration>
    void expires_after(Duration dur, callback_t&& cb)
    {
        expiry_ = Clock::now() + dur;
        start_thread(std::move(cb));
    }

    // Cancel timer threads and unregister callback
    void cancel()
    {
        cancel_thread();
    }

    time_point_t expiry() const
    {
        return expiry_;
    }

    // Blocking wait
    void wait()
    {
        std::thread([&]() {
            std::unique_lock lock(wait_cv_mtx_);
            wait_cv_.wait(lock);
        }).join();
    }

    bool canceled() const
    {
        std::lock_guard lock(cv_mtx_);
        return canceled_;
    }

private:
    void start_thread(callback_t&& cb)
    {
        if (thr_.joinable()) {
            cancel_thread();
            thr_.join();
        }
        std::lock_guard lock(cv_mtx_);
        canceled_ = false;
        thr_ = std::thread(&timer::run_thread, this, std::move(cb));
    }

    void cancel_thread()
    {
        std::lock_guard lock(cv_mtx_);
        canceled_ = true;
        cv_.notify_one();
    }

    void run_thread(callback_t&& cb)
    {
        // Wait for cancel signal or timeout
        std::unique_lock lock(cv_mtx_);
        cv_.wait_until(lock, expiry_, [&]() {
            return canceled_;
        });

        // Callback
        if (!canceled_ && cb) {
            cb();
        }

        // Signal waiters
        {
            std::lock_guard wait_lock(wait_cv_mtx_);
            wait_cv_.notify_all();
        }
    }

    std::thread thr_;
    std::mutex mutable cv_mtx_;
    std::condition_variable cv_;
    bool canceled_ {false};
    std::mutex mutable wait_cv_mtx_;
    std::condition_variable wait_cv_;
    time_point_t expiry_;
};

}

#endif //DBM_TIMER_HPP
