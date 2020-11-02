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
        conf_flags_num_items    = s_taggable + 1
    };

    model_item();

    model_item(const model_item& oth);

    model_item(model_item&& oth) = default;

    template<typename... Args>
    model_item(Args&&... args);

    virtual ~model_item() = default;

    model_item& operator=(const model_item& oth);

    model_item& operator=(model_item&& oth) = default;

    constexpr const kind::key& key() const;

    constexpr const kind::tag& tag() const;

    const container& get_container() const;

    bool is_null() const;

    bool is_defined() const;

    const kind::dbtype& dbtype() const;

    void set(const kind::key& v);

    void set(const kind::tag& v);

    void set(const kind::primary& v);

    void set(const kind::required& v);

    void set(const kind::dbtype& v);

    void set(const kind::taggable& v);

    void set(const kind::not_null& v);

    void set(const kind::auto_increment& v);

    void set(const kind::create& v);

    void set(kind::direction v);

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

    void set_() {}

    template<typename Head, typename... Tail>
    void set_(Head&& head, Tail&&... tail)
    {
        set(std::move(head));
        set_(std::forward<Tail>(tail)...);
    }

    void make_default_container();

    static constexpr unsigned conf_default =
        (1u << conf_flags::db_readable) |
        (1u << conf_flags::db_writable) |
        (1u << conf_flags::db_creatable) |
        (1u << conf_flags::s_taggable);

    kind::key key_{""};
    kind::tag tag_{""};
    std::bitset<conf_flags_num_items> conf_ {conf_default};
    kind::dbtype dbtype_ {""};
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

    constexpr auto const& cnf() const
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
    constexpr bool get(std::size_t pos) const
    {
        return cnf()[pos];
    }

public:
    explicit model_item_conf_helper(model_item const& item)
        : item_(item)
    {}

    constexpr bool readable() const
    {
        return get(model_item::db_readable);
    }

    constexpr bool writable() const
    {
        return get(model_item::db_writable);
    }

    constexpr bool creatable() const
    {
        return get(model_item::db_creatable);
    }

    constexpr bool primary() const
    {
        return get(model_item::db_pkey);
    }

    constexpr bool not_null() const
    {
        return get(model_item::db_not_null);
    }

    constexpr bool auto_increment() const
    {
        return get(model_item::db_autoincrement);
    }

    constexpr bool required() const
    {
        return get(model_item::s_required);
    }

    constexpr bool taggable() const
    {
        return get(model_item::s_taggable);
    }
};

} // namespace detail

template<typename... Args>
model_item::model_item(Args&&... args)
{
    set_(std::forward<Args>(args)...);

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
