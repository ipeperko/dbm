#ifndef DBM_MODEL_ITEM_HPP
#define DBM_MODEL_ITEM_HPP

#include "container.hpp"

namespace dbm {

class model_item
{
public:

    model_item()
    {
        make_default_container();
    }

    model_item(const model_item& oth)
        : key_(oth.key_)
        , tag_(oth.tag_)
        , primary_(oth.primary_)
        , required_(oth.required_)
        , dbtype_(oth.dbtype_)
        , taggable_(oth.taggable_)
        , direction_(oth.direction_)
        , cont_(oth.cont_->clone())
    {
    }

    model_item(model_item&& oth) = default;

    template<typename... Args>
    model_item(Args&&... args)
    {
        set_(std::forward<Args>(args)...);

        if (!cont_) {
            make_default_container();
        }
    }

    model_item& operator=(const model_item& oth)
    {
        if (this != &oth) {
            key_ = oth.key_;
            tag_ = oth.tag_;
            primary_ = oth.primary_;
            required_ = oth.required_;
            dbtype_ = oth.dbtype_;
            taggable_ = oth.taggable_;
            direction_ = oth.direction_;
            cont_ = oth.cont_->clone();
        }
        return *this;
    }

    model_item& operator=(model_item&& oth) = default;

    const kind::key& key() const
    {
        return key_;
    }

    const kind::tag& tag() const
    {
        return tag_;
    }

    const container& get_container() const
    {
        return *cont_;
    }

    bool is_primary() const
    {
        return primary_.get();
    }

    bool is_required() const
    {
        return required_.get();
    }

    bool is_taggable() const
    {
        return taggable_.get();
    }

    bool is_null() const
    {
        return !cont_ || cont_->is_null();
    }

    bool is_defined() const
    {
        return cont_ && cont_->is_defined();
    }

    const kind::dbtype& dbtype() const
    {
        return dbtype_;
    }

    kind::direction direction() const
    {
        return direction_;
    }

    void set(const kind::key& v)
    {
        key_ = v;
    }

    void set(const kind::tag& v)
    {
        tag_ = v;
    }

    void set(const kind::primary& v)
    {
        primary_ = v;
    }

    void set(const kind::required& v)
    {
        required_ = v;
    }

    void set(const kind::dbtype& v)
    {
        dbtype_ = v;
    }

    void set(const kind::taggable& v)
    {
        taggable_ = v;
    }

    void set(kind::direction v)
    {
        direction_ = v;
    }

    void set(container_ptr&& v)
    {
        cont_ = std::move(v);
    }

    void set_value(kind::variant& v)
    {
        cont_->set(v);
    }

    void set_value(kind::variant&& v)
    {
        set_value(v);
    }

    kind::variant value() const
    {
        return cont_->get();
    }

    template<typename T>
    T value() const
    {
        return cont_->get<T>();
    }

    std::string to_string() const
    {
        if (!cont_) {
            throw_exception<std::domain_error>("container is null");
        }
        return cont_->to_string();
    }

    void from_string(std::string_view v)
    {
        if (!cont_) {
            make_default_container(); // TODO: make default or throw exception?
        }

        return cont_->from_string(v);
    }

    void serialize(serializer& s)
    {
        if (!is_taggable()) {
            return;
        }

        std::string_view sv = !tag_.empty() ? tag_.get() : key_.get();

        if (!is_defined()) {
            if (is_required()) {
                throw::std::domain_error("Serializing failed - item '" + std::string(sv) + "' is required but not defined" );
            }
            return;
        }

        if (is_null()) {
            s.serialize(sv, nullptr);
        }
        else {
            cont_->serialize(s, sv);
        }
    }


    void deserialize(deserializer& s)
    {
        std::string_view sv = !tag_.empty() ? tag_.get() : key_.get();
        if (!cont_) {
            throw_exception<std::domain_error>("Cannot deserialize - no container");
        }
        cont_->deserialize(s, sv);
    }

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

    void make_default_container()
    {
        cont_ = local<std::string>();
    }

    kind::key key_{""};
    kind::tag tag_{""};
    kind::primary primary_{false};
    kind::required required_{false};
    kind::dbtype dbtype_ {""};
    kind::taggable taggable_ {true};
    kind::direction direction_{kind::direction::read_write};
    container_ptr cont_;
};

}// namespace dbm

#endif//DBM_MODEL_ITEM_HPP
