#ifndef DBM_SQL_ROWS_HPP
#define DBM_SQL_ROWS_HPP

namespace dbm::kind {

/**
 * Sql rows
 */
class sql_rows : private std::vector<sql_row>
{
    friend class sql_rows_dump;
public:
    typedef std::vector<sql_row> vector_type;

    sql_rows() = default;

    sql_rows(const sql_rows& oth);

    sql_rows(sql_rows&& oth) noexcept;

    ~sql_rows() = default;

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

    sql_rows& operator=(const sql_rows& oth);

    sql_rows& operator=(sql_rows&& oth) noexcept;

    void set_field_names(sql_fields&& f);

    sql_fields& field_names() noexcept;

    sql_fields const& field_names() const noexcept;

    sql_field_map& field_map() noexcept;

    sql_field_map const& field_map() const noexcept;

private:
    void setup_rows();

    sql_fields fnames_;
    sql_field_map fmap_;
};

DBM_INLINE sql_rows::sql_rows(const sql_rows& oth)
    : std::vector<sql_row>(oth)
    , fnames_(oth.fnames_)
    , fmap_(oth.fmap_)
{
    setup_rows();
}

DBM_INLINE sql_rows::sql_rows(sql_rows&& oth) noexcept
    : std::vector<sql_row>(std::move(oth))
    , fnames_(std::move(oth.fnames_))
    , fmap_(std::move(oth.fmap_))
{
    setup_rows();
}

DBM_INLINE sql_rows& sql_rows::operator=(const sql_rows& oth)
{
    if (this != &oth) {
        std::vector<sql_row>::operator=(oth);
        fnames_ = oth.fnames_;
        fmap_ = oth.fmap_;
        setup_rows();
    }
    return *this;
}

DBM_INLINE sql_rows& sql_rows::operator=(sql_rows&& oth) noexcept
{
    if (this != &oth) {
        std::vector<sql_row>::operator=(std::move(oth));
        fnames_ = std::move(oth.fnames_);
        fmap_ = std::move(oth.fmap_);
        setup_rows();
    }
    return *this;
}

DBM_INLINE void sql_rows::set_field_names(sql_fields&& f)
{
    fmap_.clear();
    fnames_ = std::move(f);

    for (int i = 0; i < static_cast<int>(fnames_.size()); i++) {
        fmap_[fnames_[i]] = i;
    }
}

DBM_INLINE sql_fields& sql_rows::field_names() noexcept
{
    return fnames_;
}

DBM_INLINE sql_fields const& sql_rows::field_names() const noexcept
{
    return fnames_;
}

DBM_INLINE sql_field_map& sql_rows::field_map() noexcept
{
    return fmap_;
}

DBM_INLINE sql_field_map const& sql_rows::field_map() const noexcept
{
    return fmap_;
}

DBM_INLINE void sql_rows::setup_rows()
{
    for (auto& it : *this) {
        it.set_fields(&fnames_, &fmap_);
    }
}

}

#endif //DBM_SQL_ROWS_HPP
