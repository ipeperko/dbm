#ifndef DBM_UTILS_HPP
#define DBM_UTILS_HPP

#include <stdexcept>
#include <functional>
#include <string>
#include <sstream>

#ifdef NDEBUG
#include <ostream>
#else
#include <iostream>
#endif

namespace dbm {

// ----------------------------------------------------------
// Debug msg
// ----------------------------------------------------------
#ifdef NDEBUG
#define DBM_LOG dbm::no_log()
#define DBM_LOG_ERROR dbm::no_log()

class no_log
{
public:
    template<typename T>
    no_log& operator<<(const T&)
    {
        return *this;
    }
};

#else
#define DBM_LOG std::cout
#define DBM_LOG_ERROR std::cerr
#endif

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

} // namespase utils
} // namespace dbm

#endif //DBM_UTILS_HPP
