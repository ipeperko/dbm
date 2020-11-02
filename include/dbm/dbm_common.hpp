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
struct parameter_required;
struct parameter_taggable;
struct parameter_not_null;
struct parameter_auto_increment;
struct parameter_create;
struct parameter_dbtype;
} // namespace detail

typedef detail::named_type<std::string, detail::parameter_key, detail::printable, detail::hashable> key;
typedef detail::named_type<std::string, detail::parameter_tag, detail::printable> tag;
typedef detail::named_type<bool, detail::parameter_primary, detail::printable> primary;
typedef detail::named_type<bool, detail::parameter_required, detail::printable> required;
typedef detail::named_type<bool, detail::parameter_taggable, detail::printable> taggable;
typedef detail::named_type<bool, detail::parameter_not_null, detail::printable> not_null;
typedef detail::named_type<bool, detail::parameter_auto_increment, detail::printable> auto_increment;
typedef detail::named_type<bool, detail::parameter_create, detail::printable> create;
typedef detail::named_type<std::string, detail::parameter_dbtype, detail::printable> dbtype;

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

enum class data_type : std::size_t
{
    Nullptr = 0,
    Bool,
    Int,
    Short,
    Long,
    Double,
    String,
    Char_ptr,
    String_view,
#ifdef DBM_EXPERIMENTAL_BLOB
    Blob,
#endif
    undefined = std::variant_npos,
};

static_assert(std::is_same_v<std::nullptr_t , std::variant_alternative_t< static_cast<std::size_t>(data_type::Nullptr), variant>>, "Invalid data type");
static_assert(std::is_same_v<bool, std::variant_alternative_t< static_cast<std::size_t>(data_type::Bool), variant>>, "Invalid data type");
static_assert(std::is_same_v<int, std::variant_alternative_t< static_cast<std::size_t>(data_type::Int), variant>>, "Invalid data type");
static_assert(std::is_same_v<short, std::variant_alternative_t< static_cast<std::size_t>(data_type::Short), variant>>, "Invalid data type");
static_assert(std::is_same_v<long, std::variant_alternative_t< static_cast<std::size_t>(data_type::Long), variant>>, "Invalid data type");
static_assert(std::is_same_v<double, std::variant_alternative_t< static_cast<std::size_t>(data_type::Double), variant>>, "Invalid data type");
static_assert(std::is_same_v<std::string, std::variant_alternative_t< static_cast<std::size_t>(data_type::String), variant>>, "Invalid data type");
static_assert(std::is_same_v<char const*, std::variant_alternative_t< static_cast<std::size_t>(data_type::Char_ptr), variant>>, "Invalid data type");
static_assert(std::is_same_v<std::string_view, std::variant_alternative_t< static_cast<std::size_t>(data_type::String_view), variant>>, "Invalid data type");
#ifdef DBM_EXPERIMENTAL_BLOB
static_assert(std::is_same_v<blob, std::variant_alternative_t< static_cast<std::size_t>(data_type::Blob), variant>>, "Invalid data type");
#endif

namespace detail {

inline constexpr std::size_t variant_size = std::variant_size_v<variant>;

template<std::size_t idx, typename T>
std::size_t variant_index_search()
{
    if constexpr (idx >= variant_size) {
        return std::variant_npos;
    }
    else if constexpr (std::is_same_v<T, std::variant_alternative_t<idx, variant>>) {
        return idx;
    }
    else {
        return variant_index_search<idx + 1, T>();
    }
}

template<typename T>
std::size_t variant_index()
{
    return variant_index_search<0, T>();
}

} // namespace detail

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
