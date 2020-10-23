#ifndef DBM_SQL_ROWS_HPP
#define DBM_SQL_ROWS_HPP

namespace dbm::kind {

/**
 * Sql rows
 */
class sql_rows : public std::vector<sql_row>
{
    friend class sql_rows_dump;
public:
    sql_rows() = default;

    virtual ~sql_rows() = default;

    sql_rows(const sql_rows& oth);

    sql_rows(sql_rows&& oth) noexcept;

    sql_rows& operator=(const sql_rows& oth);

    sql_rows& operator=(sql_rows&& oth) noexcept;

    void set_field_names(sql_fields&& f);

    sql_fields& field_names();

    sql_fields const& field_names() const;

    sql_field_map& field_map();

    sql_field_map const& field_map() const;

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

DBM_INLINE sql_fields& sql_rows::field_names()
{
    return fnames_;
}

DBM_INLINE sql_fields const& sql_rows::field_names() const
{
    return fnames_;
}

DBM_INLINE sql_field_map& sql_rows::field_map()
{
    return fmap_;
}

DBM_INLINE sql_field_map const& sql_rows::field_map() const
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
