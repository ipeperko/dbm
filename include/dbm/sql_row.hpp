#ifndef DBM_SQL_ROW_HPP
#define DBM_SQL_ROW_HPP

namespace dbm::kind {

/**
 * Sql row
 */
class sql_row : private std::vector<sql_value>
{
public:
    typedef std::vector<sql_value> vector_type;

    sql_row() = default;

    ~sql_row() = default;

    // vector_type member types
    using vector_type::iterator;
    using vector_type::const_iterator;
    using vector_type::reverse_iterator;
    using vector_type::const_reverse_iterator;
    // vector_type member functions
    using vector_type::assign;
    // vector_type element access
    using vector_type::at;
    using vector_type::operator[];
    using vector_type::front;
    using vector_type::back;
    using vector_type::data;
    // vector_type iterators
    using vector_type::begin;
    using vector_type::end;
    using vector_type::cbegin;
    using vector_type::cend;
    using vector_type::crbegin;
    using vector_type::crend;
    // vector_type capacity
    using vector_type::empty;
    using vector_type::size;
    using vector_type::max_size;
    using vector_type::reserve;
    using vector_type::capacity;
    using vector_type::shrink_to_fit;
    // vector_type modifiers
    using vector_type::clear;
    using vector_type::insert;
    using vector_type::emplace;
    using vector_type::erase;
    using vector_type::push_back;
    using vector_type::emplace_back;
    using vector_type::pop_back;
    using vector_type::resize;
    // no swap

    void set_fields(kind::sql_fields* field_names, sql_field_map* field_map) noexcept;

    sql_value& at(std::string const& key);

    sql_value const& at(std::string const& key) const;

    sql_fields* field_names() noexcept;

    sql_fields const* field_names() const noexcept;

    sql_field_map* field_map() noexcept;

    sql_field_map const* field_map() const noexcept;

private:
    sql_fields* fnames_ { nullptr };
    sql_field_map* fmap_ { nullptr };
};

DBM_INLINE void sql_row::set_fields(kind::sql_fields* field_names, sql_field_map* field_map) noexcept
{
    fnames_ = field_names;
    fmap_ = field_map;
}

DBM_INLINE sql_value& sql_row::at(std::string const& key)
{
    auto it = fmap_->find(key);
    if (it != fmap_->end()) {
        return at(it->second);
    }
    throw_exception<std::out_of_range>("sql row item not found " + std::string(key));
}

DBM_INLINE sql_value const& sql_row::at(std::string const& key) const
{
    auto it = fmap_->find(key);
    if (it != fmap_->end()) {
        return at(it->second);
    }
    throw_exception<std::out_of_range>("sql row item not found " + std::string(key));
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
