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

    sql_value(sql_value&& oth) noexcept
        : value_(oth.value_)
        , len_(oth.len_)
    {
        oth.value_ = nullptr;
        oth.len_ = 0;
    }

    sql_value(const char* v, size_t len)
        : value_(v)
        , len_(len)
    {}

    sql_value& operator=(const sql_value&) = default;

    sql_value& operator=(sql_value&& oth) noexcept
    {
        if (this != &oth) {
            value_ = oth.value_;
            len_ = oth.len_;
            oth.value_ = nullptr;
            oth.len_ = 0;
        }
        return *this;
    }

    bool operator==(const sql_value& oth) const
    {
        return (value_ == oth.value_ && len_ == oth.len_); // values are equal if both points to same location
    }

    bool null() const
    {
        return !value_;
    }

    size_t length() const
    {
        return len_;
    }

    std::string_view get() const
    {
        if (!value_) {
            throw_exception<std::domain_error>("sql_value is null");
        }

        return { value_, len_ };
    }

    template<typename T>
    T get() const
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

    std::string_view get_optional(std::string_view opt) const
    {
        if (null()) {
            return opt;
        }

        return { value_, len_ };
    }

    // TODO: specialize for std::string: if T=std::string opt is constructed from const char* or string_view
    template<typename T>
    T get_optional(T const& opt) const
    {
        if (null()) {
            return opt;
        }

        bool ok;
        T val = get_no_null_check<T>(ok);
        return ok ? val : opt;
    }

    friend std::ostream& operator<<(std::ostream& os, const sql_value& v)
    {
        os << (v.null() ? "NULL" : v.get());
        return os;
    }

private:
    template<typename T>
    T get_no_null_check(bool& ok) const
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
                //throw_exception<std::domain_error>("sql_value conversion error");
            }
            else {
                ok = true;
            }
            return cv;
        }
    }

    const char* value_ { nullptr };
    size_t len_ { 0 };
};

}

#endif //DBM_SQL_VALUE_HPP
