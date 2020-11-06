#ifndef DBM_SQL_ROW_HPP
#define DBM_SQL_ROW_HPP

namespace dbm::kind {

/**
 * Sql row
 */
class sql_row : public std::vector<sql_value>
{
public:
    sql_row() = default;

    ~sql_row() = default;

    void set_fields(kind::sql_fields* field_names, sql_field_map* field_map);

    sql_value& at(std::string_view key);

    sql_value const& at(std::string_view key) const;

    sql_value& at(int idx);

    sql_value const& at(int idx) const;

    sql_fields* field_names() noexcept;

    sql_fields const* field_names() const noexcept;

    sql_field_map* field_map() noexcept;

    sql_field_map const* field_map() const noexcept;

private:
    sql_fields* fnames_ { nullptr };
    sql_field_map* fmap_ { nullptr };
};

DBM_INLINE void sql_row::set_fields(kind::sql_fields* field_names, sql_field_map* field_map)
{
    fnames_ = field_names;
    fmap_ = field_map;
}

DBM_INLINE sql_value& sql_row::at(std::string_view key)
{
    for (auto& it : *fmap_) {
        if (it.first == key) {
            return at(it.second);
        }
    }

    throw_exception<std::out_of_range>("sql row item not found " + std::string(key));
}

DBM_INLINE sql_value const& sql_row::at(std::string_view key) const
{
    for (auto& it : *fmap_) {
        if (it.first == key) {
            return at(it.second);
        }
    }

    throw_exception<std::out_of_range>("sql row item not found " + std::string(key));
}

DBM_INLINE sql_value& sql_row::at(int idx)
{
    return std::vector<sql_value>::at(idx);
}

DBM_INLINE sql_value const& sql_row::at(int idx) const
{
    return std::vector<sql_value>::at(idx);
}

DBM_INLINE sql_fields* sql_row::field_names() noexcept
{
    return fnames_;
}

DBM_INLINE sql_fields const* sql_row::field_names() const noexcept
{
    return fnames_;
}

DBM_INLINE sql_field_map* sql_row::field_map() noexcept
{
    return fmap_;
}

DBM_INLINE sql_field_map const* sql_row::field_map() const noexcept
{
    return fmap_;
}

}

#endif //DBM_SQL_ROW_HPP
