#ifndef DBM_SQL_TYPES_HPP
#define DBM_SQL_TYPES_HPP

#include "dbm_common.hpp"
#include <sstream>

namespace dbm {
namespace kind {

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
    size_t len_ {0};
};

/**
 * Sql row
 */
class sql_row : public std::vector<sql_value>
{
public:
    sql_row()
    {}

    void set_fields(kind::sql_fields* field_names, sql_field_map* field_map)
    {
        fnames_ = field_names;
        fmap_ = field_map;
    }

    sql_value& at(std::string_view key)
    {
        for (auto& it : *fmap_) {
            if (it.first == key) {
                return at(it.second);
            }
        }

        throw_exception<std::out_of_range>("sql row item not found " + std::string(key));
    }

    sql_value const& at(std::string_view key) const
    {
        for (auto& it : *fmap_) {
            if (it.first == key) {
                return at(it.second);
            }
        }

        throw_exception<std::out_of_range>("sql row item not found " + std::string(key));
    }

    /*
    sql_value& at(const kind::key& key)
    {
        return at(fmap_->at(key));
    }

    sql_value const& at(const kind::key& key) const
    {
        return at(fmap_->at(key));
    }
     */

    sql_value& at(int idx)
    {
        return std::vector<sql_value>::at(idx);
    }

    sql_value const& at(int idx) const
    {
        return std::vector<sql_value>::at(idx);
    }

    sql_fields* field_names()
    {
        return fnames_;
    }

    sql_fields const* field_names() const
    {
        return fnames_;
    }

    sql_field_map* field_map()
    {
        return fmap_;
    }

    sql_field_map const* field_map() const
    {
        return fmap_;
    }

private:
    sql_fields* fnames_ { nullptr };
    sql_field_map* fmap_ { nullptr };
};

/**
 * Sql rows
 */
class sql_rows : public std::vector<sql_row>
{
    friend class sql_rows_dump;
public:
    sql_rows() = default;

    sql_rows(const sql_rows& oth)
        : std::vector<sql_row>(oth)
        , fnames_(oth.fnames_)
        , fmap_(oth.fmap_)
    {
        setup_rows();
    }

    sql_rows(sql_rows&& oth)
        : std::vector<sql_row>(std::move(oth))
        , fnames_(std::move(oth.fnames_))
        , fmap_(std::move(oth.fmap_))
    {
        setup_rows();
    }

    sql_rows& operator=(const sql_rows& oth)
    {
        if (this != &oth) {
            std::vector<sql_row>::operator=(oth);
            fnames_ = oth.fnames_;
            fmap_ = oth.fmap_;
            setup_rows();
        }
        return *this;
    }

    sql_rows& operator=(sql_rows&& oth)
    {
        if (this != &oth) {
            std::vector<sql_row>::operator=(std::move(oth));
            fnames_ = std::move(oth.fnames_);
            fmap_ = std::move(oth.fmap_);
            setup_rows();
        }
        return *this;
    }

    void set_field_names(sql_fields&& f)
    {
        fmap_.clear();
        fnames_ = std::move(f);

        for (int i = 0; i < static_cast<int>(fnames_.size()); i++) {
            fmap_[fnames_[i]] = i;
        }
    }

    sql_fields& field_names()
    {
        return fnames_;
    }

    sql_fields const& field_names() const
    {
        return fnames_;
    }

    sql_field_map& field_map()
    {
        return fmap_;
    }

    sql_field_map const& field_map() const
    {
        return fmap_;
    }

private:
    void setup_rows()
    {
        for (auto& it : *this) {
            it.set_fields(&fnames_, &fmap_);
        }
    }

    sql_fields fnames_;
    sql_field_map fmap_;
};

/**
 * Sql rows dump
 */
class sql_rows_dump
{
public:
    sql_rows_dump() = default;

    sql_rows_dump(const sql_rows& src) // implicit is desired
    {
        store(src);
    }

    sql_rows_dump& operator=(const sql_rows& src)
    {
        store(src);
        return *this;
    }

    sql_rows_dump& operator+=(const sql_rows& src)
    {
        if (src.field_map() != fmap_ && src.field_names() != fnames_) {
            throw_exception<std::domain_error>("Cannot append rows with different field names");
        }
        append(src);
        return *this;
    }

    void store(const sql_rows& src)
    {
        data_.clear();
        fnames_ = src.field_names();
        fmap_ = src.field_map();
        //        size_t n = src.size() * field_names_.size();
        //        data_.reserve(n);
        append(src);
    }

    void restore(sql_rows& dest)
    {
        dest.clear();
        dest.set_field_names(sql_fields(fnames_));

        for (const auto& row : data_) {
            auto& dest_row = dest.emplace_back();
            for (const auto& val : row) {
                if (val)
                    dest_row.emplace_back(val->data(), val->size());
                else
                    dest_row.emplace_back(nullptr, 0);
            }
        }

        dest.setup_rows();
    }

    sql_rows restore()
    {
        sql_rows rows;
        restore(rows);
        return rows;
    }

    sql_fields const& field_names() const
    {
        return fnames_;
    }

    sql_field_map const& field_map() const
    {
        return fmap_;
    }

    bool empty() const
    {
        return data_.empty();
    }

    void clear()
    {
        data_.clear();
        data_.shrink_to_fit();
        fnames_.clear();
        fmap_.clear();
    }

private:
    void append(const sql_rows& src)
    {
        for (auto const& src_row : src) {
            auto& row = data_.emplace_back();

            for (auto const& src_val : src_row) {
                if (src_val.null()) {
                    row.emplace_back(std::nullopt);
                }
                else {
                    std::string s(src_val.get().data(), src_val.length());
                    row.emplace_back(std::move(s));
                }
            }
        }
    }

    sql_fields fnames_;
    sql_field_map fmap_;
    std::vector<std::vector<std::optional<std::string>>> data_;
};

} // namespace kind
} // namespace dbm

#endif //DBM_SQL_TYPES_HPP
