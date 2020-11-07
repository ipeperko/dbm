#ifndef DBM_COMMON_HPP
#define DBM_COMMON_HPP

#include <dbm/dbm_config.hpp>
#include <dbm/dbm_export.hpp>
#include <dbm/detail/utils.hpp>
#include <dbm/detail/default_constraint.hpp>
#include <dbm/detail/named_type.hpp>
#include <dbm/detail/statement.hpp>
#include <dbm/detail/timestamp2u_converter.hpp>
#include <functional>
#include <memory>
#include <stdexcept>
#include <variant>
#include <vector>

namespace dbm {

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
struct parameter_valquotes;
struct parameter_dbtype;
} // namespace detail

using key = detail::named_type<std::string, detail::parameter_key, detail::printable, detail::hashable>;
using tag = detail::named_type<std::string, detail::parameter_tag, detail::printable>;
using primary = detail::named_type<bool, detail::parameter_primary, detail::printable, detail::operator_bool>;
using required = detail::named_type<bool, detail::parameter_required, detail::printable, detail::operator_bool>;
using taggable = detail::named_type<bool, detail::parameter_taggable, detail::printable, detail::operator_bool>;
using not_null = detail::named_type<bool, detail::parameter_not_null, detail::printable, detail::operator_bool>;
using auto_increment = detail::named_type<bool, detail::parameter_auto_increment, detail::printable, detail::operator_bool>;
using create = detail::named_type<bool, detail::parameter_create, detail::printable, detail::operator_bool>;
using valquotes = detail::named_type<bool, detail::parameter_valquotes, detail::printable, detail::operator_bool>;
using defaultc = kind::detail::default_constraint;
using custom_data_type = detail::named_type<std::string, detail::parameter_dbtype, detail::printable>;

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
                             int32_t,
                             int16_t,
                             int64_t,
                             uint32_t,
                             uint16_t,
                             uint64_t,
                             detail::timestamp2u_converter,
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
    Nullptr = 0,    /* Exists only to pass null */
    Bool,
    Int32,
    Int16,
    Int64,
    UInt32,
    UInt16,
    Uint64,
    Timestamp2u,
    Double,
    String,
    Char_ptr,       /* Exists only to construct string values */
    String_view,    /* Exists only to construct string values */
#ifdef DBM_EXPERIMENTAL_BLOB
    Blob,
#endif
    undefined = std::variant_npos,
};

static_assert(std::is_same_v<std::nullptr_t , std::variant_alternative_t< static_cast<std::size_t>(data_type::Nullptr), variant>>, "Invalid data type");
static_assert(std::is_same_v<bool, std::variant_alternative_t< static_cast<std::size_t>(data_type::Bool), variant>>, "Invalid data type");
static_assert(std::is_same_v<int, std::variant_alternative_t< static_cast<std::size_t>(data_type::Int32), variant>>, "Invalid data type");
static_assert(std::is_same_v<short, std::variant_alternative_t< static_cast<std::size_t>(data_type::Int16), variant>>, "Invalid data type");
static_assert(std::is_same_v<long, std::variant_alternative_t< static_cast<std::size_t>(data_type::Int64), variant>>, "Invalid data type");
static_assert(std::is_same_v<unsigned int, std::variant_alternative_t< static_cast<std::size_t>(data_type::UInt32), variant>>, "Invalid data type");
static_assert(std::is_same_v<unsigned short, std::variant_alternative_t< static_cast<std::size_t>(data_type::UInt16), variant>>, "Invalid data type");
static_assert(std::is_same_v<unsigned long, std::variant_alternative_t< static_cast<std::size_t>(data_type::Uint64), variant>>, "Invalid data type");
static_assert(std::is_same_v<detail::timestamp2u_converter, std::variant_alternative_t< static_cast<std::size_t>(data_type::Timestamp2u), variant>>, "Invalid data type");
static_assert(std::is_same_v<double, std::variant_alternative_t< static_cast<std::size_t>(data_type::Double), variant>>, "Invalid data type");
static_assert(std::is_same_v<std::string, std::variant_alternative_t< static_cast<std::size_t>(data_type::String), variant>>, "Invalid data type");
static_assert(std::is_same_v<char const*, std::variant_alternative_t< static_cast<std::size_t>(data_type::Char_ptr), variant>>, "Invalid data type");
static_assert(std::is_same_v<std::string_view, std::variant_alternative_t< static_cast<std::size_t>(data_type::String_view), variant>>, "Invalid data type");
#ifdef DBM_EXPERIMENTAL_BLOB
static_assert(std::is_same_v<blob, std::variant_alternative_t< static_cast<std::size_t>(data_type::Blob), variant>>, "Invalid data type");
#endif

static_assert(std::is_same_v<time_t, int64_t> or std::is_same_v<time_t, uint64_t> or std::is_same_v<time_t, double>, "time_t not portable");

namespace detail {

inline constexpr std::size_t variant_size = std::variant_size_v<variant>;

template<std::size_t idx, typename T>
constexpr std::size_t variant_index_search() noexcept
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
constexpr std::size_t variant_index() noexcept
{
    return variant_index_search<0, T>();
}

} // namespace detail

} // namespace kind

// ----------------------------------------------------------
// Utils
// ----------------------------------------------------------

namespace utils {

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

} // namespace utils
} // namespace dbm

#endif //DBM_COMMON_HPP
