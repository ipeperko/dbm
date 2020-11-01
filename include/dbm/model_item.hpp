#ifndef DBM_MODEL_ITEM_HPP
#define DBM_MODEL_ITEM_HPP

#include <dbm/container.hpp>
#include <bitset>

namespace dbm {

class model_item
{
public:

    enum conf_flags : unsigned
    {
        db_readable             = 0,
        db_writable             = 1,
        db_pkey                 = 2,
        db_not_null             = 3,
        db_autoincrement        = 4,
        s_required              = 5,
        s_taggable              = 6,
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

    constexpr bool is_primary() const;

    constexpr bool is_required() const;

    constexpr bool is_taggable() const;

    bool is_null() const;

    bool is_defined() const;

    constexpr bool is_writable() const;

    constexpr bool is_readable() const;

    const kind::dbtype& dbtype() const;

    void set(const kind::key& v);

    void set(const kind::tag& v);

    void set(const kind::primary& v);

    void set(const kind::required& v);

    void set(const kind::dbtype& v);

    void set(const kind::taggable& v);

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

    kind::key key_{""};
    kind::tag tag_{""};
    std::bitset<conf_flags_num_items> conf_ {(1u << db_readable) | (1u << db_writable) | (1u << s_taggable)};
    kind::dbtype dbtype_ {""};
    container_ptr cont_;
};

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
