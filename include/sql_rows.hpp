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

}

#endif //DBM_SQL_ROWS_HPP
