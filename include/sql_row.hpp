#ifndef DBM_SQL_ROW_HPP
#define DBM_SQL_ROW_HPP

#include "sql_value.hpp"

namespace dbm::kind {

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

}

#endif //DBM_SQL_ROW_HPP
