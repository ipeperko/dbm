#ifndef DBM_SQL_ROWS_DUMP_HPP
#define DBM_SQL_ROWS_DUMP_HPP

#include "sql_rows.hpp"

namespace dbm::kind {

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

    size_t size() const
    {
        return data_.size();
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

}

#endif //DBM_SQL_ROWS_DUMP_HPP
