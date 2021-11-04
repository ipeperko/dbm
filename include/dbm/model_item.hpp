#ifndef DBM_MODEL_ITEM_HPP
#define DBM_MODEL_ITEM_HPP

#include <dbm/container.hpp>
#include <bitset>

namespace dbm {

namespace detail {
class model_item_conf_helper;
}

/*!
 * Model item class
 */
class model_item
{
    friend class detail::model_item_conf_helper;
public:

    enum conf_flags : unsigned
    {
        db_readable             = 0,
        db_writable             = 1,
        db_creatable = 2,
        db_pkey                 = 3,
        db_not_null             = 4,
        db_autoincrement        = 5,
        s_required              = 6,
        s_taggable              = 7,
        w_quotes                = 8,
        conf_flags_num_items    = w_quotes + 1
    };

    model_item();

    model_item(const model_item& oth);

    model_item(model_item&& oth) = default;

    template<typename... Args>
    model_item(Args&&... args);

    virtual ~model_item() = default;

    model_item& operator=(const model_item& oth);

    model_item& operator=(model_item&& oth) = default;

    constexpr const kind::key& key() const noexcept;

    constexpr const kind::tag& tag() const noexcept;

    container& get_container() __attribute_deprecated__;

    const container& get_container() const __attribute_deprecated__;

    container& get();

    const container& get() const;

    container& operator*();

    container const& operator*() const;

    bool is_null() const noexcept;

    bool is_defined() const noexcept;

    const kind::custom_data_type& custom_data_type() const noexcept;

    void set(const kind::key& v);

    void set(const kind::tag& v);

    void set(kind::primary v);

    void set(kind::required v);

    void set(const kind::custom_data_type& v);

    void set(kind::taggable v);

    void set(kind::not_null v);

    void set(kind::auto_increment v);

    void set(kind::create v);

    void set(const kind::defaultc& v);

    void set(kind::direction v);

    void set(kind::valquotes v);

    void set(container_ptr&& v);

    void set_value(kind::variant& v);

    void set_value(kind::variant&& v);

    kind::variant value() const;

    template<typename T>
    T value() const;

    std::string to_string() const;

    void from_string(std::string_view v);

    void serialize(serializer& s);

    void deserialize(deserializer& s);

    detail::model_item_conf_helper conf() const;

private:

    void set(const model_item& oth)
    {
        *this = oth;
    }

    void set_helper() {}

    template<typename Head, typename... Tail>
    void set_helper(Head&& head, Tail&&... tail)
    {
        set(std::forward<Head>(head));
        set_helper(std::forward<Tail>(tail)...);
    }

    void make_default_container();

    static constexpr unsigned conf_default =
        (1u << conf_flags::db_readable) |
        (1u << conf_flags::db_writable) |
        (1u << conf_flags::db_creatable) |
        (1u << conf_flags::s_taggable) |
        (1u << conf_flags::w_quotes);

    kind::key key_{""};
    kind::tag tag_{""};
    std::bitset<conf_flags_num_items> conf_ {conf_default};
    kind::custom_data_type dbtype_ {""};
    kind::defaultc defaultc_;
    container_ptr cont_;
};

namespace detail {

/*!
 * Helper class for model item configuration bits reading
 *
 * The main reason for this class is to avoid confusion between
 * value state 'is_null' and configuration flag 'not_null'.
 *
 */
class model_item_conf_helper
{
    model_item const& item_;

    constexpr auto const& cnf() const noexcept
    {
        return item_.conf_;
    }

    /*!
     * Get flag value
     *
     * Note that this method should be private as we don't perform bounds checking!
     *
     * @param pos flag position
     * @return flag state
     */
    constexpr bool get(std::size_t pos) const noexcept
    {
        return cnf()[pos];
    }

public:
    explicit model_item_conf_helper(model_item const& item)
        : item_(item)
    {}

    constexpr bool readable() const noexcept
    {
        return get(model_item::db_readable);
    }

    constexpr bool writable() const noexcept
    {
        return get(model_item::db_writable);
    }

    constexpr bool creatable() const noexcept
    {
        return get(model_item::db_creatable);
    }

    constexpr bool primary() const noexcept
    {
        return get(model_item::db_pkey);
    }

    constexpr bool not_null() const noexcept
    {
        return get(model_item::db_not_null);
    }

    constexpr bool auto_increment() const noexcept
    {
        return get(model_item::db_autoincrement);
    }

    constexpr bool required() const noexcept
    {
        return get(model_item::s_required);
    }

    constexpr bool taggable() const noexcept
    {
        return get(model_item::s_taggable);
    }

    constexpr bool valquotes() const noexcept
    {
        return get(model_item::w_quotes);
    }

    const kind::defaultc& default_constraint() const noexcept
    {
        return item_.defaultc_;
    }
};

} // namespace detail

template<typename... Args>
model_item::model_item(Args&&... args)
{
    set_helper(std::forward<Args>(args)...);

    if (!cont_) {
        make_default_container();
    }
}

template<typename T>
T model_item::value() const
{
    return cont_->get<T>();
}

}// namespace dbm

#endif//DBM_MODEL_ITEM_HPP
