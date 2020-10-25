#ifndef DBM_SQL_VALUE_HPP
#define DBM_SQL_VALUE_HPP

#include "dbm_common.hpp"

namespace dbm::kind {

/**
 * Sql field names vector
 */
typedef std::vector<std::string> sql_fields;

/**
 * Sql field names map
 * @first key
 * @second vector index
 */
typedef std::unordered_map<std::string, int> sql_field_map;

/**
 * Sql value
 */
class sql_value
{
public:
    sql_value() = default;

    sql_value(const sql_value&) = default;

    sql_value(sql_value&& oth) noexcept;

    sql_value(const char* v, size_t len);

    virtual ~sql_value() = default;

    sql_value& operator=(const sql_value&) = default;

    sql_value& operator=(sql_value&& oth) noexcept;

    bool operator==(const sql_value& oth) const;

    explicit operator bool() const;

    bool null() const;

    size_t length() const;

    std::string_view get() const;

    template<typename T>
    T get() const;

    std::string_view get_optional(std::string_view opt) const;

    // TODO: specialize for std::string: if T=std::string opt is constructed from const char* or string_view
    template<typename T>
    T get_optional(T const& opt) const;

    friend std::ostream& operator<<(std::ostream& os, const sql_value& v)
    {
        if (v.null()) {
            os << "NULL";
        }
        else if (v.len_) {
            os << v.get();
        }
        // else: ignore printing empty string

        return os;
    }

private:
    template<typename T>
    T get_no_null_check(bool& ok) const;

    const char* value_ { nullptr };
    size_t len_ { 0 };
};

DBM_INLINE sql_value::sql_value(sql_value&& oth) noexcept
    : value_(oth.value_)
    , len_(oth.len_)
{
    oth.value_ = nullptr;
    oth.len_ = 0;
}

DBM_INLINE sql_value::sql_value(const char* v, size_t len)
    : value_(v)
    , len_(len)
{}


DBM_INLINE sql_value& sql_value::operator=(sql_value&& oth) noexcept
{
    if (this != &oth) {
        value_ = oth.value_;
        len_ = oth.len_;
        oth.value_ = nullptr;
        oth.len_ = 0;
    }
    return *this;
}

DBM_INLINE bool sql_value::operator==(const sql_value& oth) const
{
    return (value_ == oth.value_ && len_ == oth.len_); // values are equal if both points to same location
}

DBM_INLINE sql_value::operator bool() const
{
    return value_ != nullptr;
}

DBM_INLINE bool sql_value::null() const
{
    return !value_;
}

DBM_INLINE size_t sql_value::length() const
{
    return len_;
}

DBM_INLINE std::string_view sql_value::get() const
{
    if (!value_) {
        throw_exception<std::domain_error>("sql_value is null");
    }

    return { value_, len_ };
}

template<typename T>
T sql_value::get() const
{
    if (null()) {
        throw_exception<std::domain_error>("sql_value is null");
    }

    bool ok;
    T val = get_no_null_check<T>(ok);
    if (!ok) {
        throw_exception<std::domain_error>("sql_value conversion error");
    }

    return val;
}

DBM_INLINE std::string_view sql_value::get_optional(std::string_view opt) const
{
    if (null()) {
        return opt;
    }

    return { value_, len_ };
}

template<typename T>
T sql_value::get_optional(T const& opt) const
{
    if (null()) {
        return opt;
    }

    bool ok;
    T val = get_no_null_check<T>(ok);
    return ok ? val : opt;
}

template<typename T>
T sql_value::get_no_null_check(bool& ok) const
{
    if constexpr (std::is_same_v<std::string, T>) {
        ok = true;
        return std::string(value_);
    }
    else {
        std::istringstream ss(value_);
        T cv;
        ss >> cv;
        if (!ss.eof() || ss.fail()) {
            ok = false;
        }
        else {
            ok = true;
        }
        return cv;
    }
}

}

#endif //DBM_SQL_VALUE_HPP
