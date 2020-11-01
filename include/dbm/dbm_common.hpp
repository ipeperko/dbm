#ifndef DBM_COMMON_HPP
#define DBM_COMMON_HPP

#include <dbm/dbm_config.hpp>
#include <dbm/dbm_export.hpp>
#include <dbm/detail/named_type.hpp>
#include <functional>
#include <memory>
#include <stdexcept>
#include <variant>
#include <vector>

#define DBM_INLINE inline

#ifdef NDEBUG
#include <ostream>
#else
#include <iostream>
#endif

namespace dbm {

// ----------------------------------------------------------
// Exception
// ----------------------------------------------------------

namespace config
{

namespace detail
{
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
}

template <typename ExceptionType = std::domain_error>
[[noreturn]] DBM_EXPORT void throw_exception(std::string const& what)
{
    if (auto& fn = config::get_custom_exception()) {
        fn(what); // should not return
    }

    throw ExceptionType(what); // if custom exception not exists or didn't throw
}


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
// Type definitions
// ----------------------------------------------------------

namespace kind {

namespace detail {
struct parameter_key;
struct parameter_tag;
struct parameter_primary;
struct parameter_reqired;
struct parameter_dbtype;
struct parameter_null;
struct parameter_taggable;
} // namespace detail

typedef detail::named_type<std::string, detail::parameter_key, detail::printable, detail::hashable> key;
typedef detail::named_type<std::string, detail::parameter_tag, detail::printable> tag;
typedef detail::named_type<bool, detail::parameter_primary, detail::printable> primary;
typedef detail::named_type<bool, detail::parameter_reqired, detail::printable> required;
typedef detail::named_type<std::string, detail::parameter_dbtype, detail::printable> dbtype;
typedef detail::named_type<bool, detail::parameter_null, detail::printable> null;
typedef detail::named_type<bool, detail::parameter_taggable, detail::printable> taggable;

enum class direction : unsigned
{
    disabled,
    read_only,
    write_only,
    bidirectional,
};

#ifdef DBM_EXPERIMENTAL_BLOB
typedef std::vector<char> blob;
#endif

using variant = std::variant<std::nullptr_t,
                             bool,
                             int,
                             short,
                             long,
                             double,
                             std::string,
                             const char*,
                             std::string_view
#ifdef DBM_EXPERIMENTAL_BLOB
                             , kind::blob
#endif
                             >;

} // namespace kind

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

template<typename Tcast, typename T>
bool arithmetic_convert(T& dest, const kind::variant& var)
{
    if (std::holds_alternative<Tcast>(var)) {
        dest = narrow_cast<T>(std::get<Tcast>(var));
        return true;
    }
    else {
        return false;
    }
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

} // namespace utils
} // namespace dbm

#endif //DBM_COMMON_HPP
