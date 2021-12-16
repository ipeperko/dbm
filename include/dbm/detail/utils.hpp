#ifndef DBM_UTILS_HPP
#define DBM_UTILS_HPP

#include <stdexcept>
#include <functional>
#include <string>
#include <sstream>

#define DBM_INLINE inline
#if __cplusplus > 201703L
#define DBM_LIKELY [[likely]]
#define DBM_UNLIKELY [[unlikely]]
#else
#define DBM_LIKELY
#define DBM_UNLIKELY
#endif

#include <iostream>

namespace dbm {

// ----------------------------------------------------------
// Exception
// ----------------------------------------------------------

namespace config {

namespace detail {
inline std::function<void(std::string const&)> custom_except_fn;
}

DBM_EXPORT inline void set_custom_exception(std::function<void(std::string const&)> except_fn)
{
    detail::custom_except_fn = std::move(except_fn);
}

DBM_EXPORT inline std::function<void(std::string const&)>& get_custom_exception()
{
    return detail::custom_except_fn;
}
} // namespace config

template<typename ExceptionType = std::domain_error>
[[noreturn]] DBM_EXPORT void throw_exception(std::string const& what)
{
    if (auto& fn = config::get_custom_exception()) {
        fn(what); // should not return
    }

    throw ExceptionType(what); // if custom exception not exists or didn't throw
}

// ----------------------------------------------------------
// Utils
// ----------------------------------------------------------

namespace utils {

template<typename T>
struct is_string_type : std::integral_constant<bool, std::is_constructible<std::string, T>::value>
{
};

template<typename T1, typename T2>
T1 narrow_cast(const T2& v2)
{
    T1 casted = static_cast<T1>(v2);
    T2 casted_2 = static_cast<T2>(casted);
    if (v2 != casted_2) {
        throw_exception<std::domain_error>("narrow casting failed");
    }

    return casted;
}

// ----------------------------------------------------------
// Input stream with ext buffer
// ----------------------------------------------------------

class DBM_EXPORT istream_extbuf : public std::istream
{
public:
    istream_extbuf(char* s, size_t n)
        : std::istream(&sb_)
    {
        sb_.pubsetbuf(s, n);
    }

private:
    std::stringbuf sb_;
};

// ----------------------------------------------------------
// Execute at function exit
// ----------------------------------------------------------

template <typename Fn>
class execute_at_exit
{
    Fn fn_;
    bool canceled_ {false};
public:
    explicit execute_at_exit(Fn&& fn)
        : fn_(std::move(fn))
    {}

    ~execute_at_exit()
    {
        if (!canceled_)
            fn_();
    }

    void cancel() { canceled_ = true; }
};

// ----------------------------------------------------------
// Debug logger
// ----------------------------------------------------------

class DBM_EXPORT debug_logger
{
public:

    enum class level
    {
        Debug,
        Error
    };

    explicit debug_logger(level lv)
        : lv_(lv)
    {
    }

    ~debug_logger()
    {
        if (writer) {
            writer(lv_, os_.str());
        }
    }

    template<typename T>
    debug_logger& operator<<([[maybe_unused]] T const& val)
    {
        if (writer) {
            os_ << val;
        }

        return *this;
    }

    static std::function<void(level, std::string&&)> writer;
    static std::function<void(level, std::string&&)> const stdout_writer;

private:
    std::ostringstream os_;
    level lv_;
};

inline std::function<void(debug_logger::level, std::string&&)> debug_logger::writer;

inline std::function<void(debug_logger::level, std::string&&)> const debug_logger::stdout_writer = [](debug_logger::level lv, std::string&& msg) {
    if (lv < level::Error) {
        std::cout << msg << std::endl;
    }
    else {
        std::cerr << msg << std::endl;
    }
};

} // namespase utils
} // namespace dbm

#endif //DBM_UTILS_HPP
