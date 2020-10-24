#ifndef DBM_MODEL_ITEM_IPP
#define DBM_MODEL_ITEM_IPP

namespace dbm {

DBM_INLINE model_item::model_item()
{
    make_default_container();
}

DBM_INLINE model_item::model_item(const model_item& oth)
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

DBM_INLINE model_item& model_item::operator=(const model_item& oth)
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

DBM_INLINE const kind::key& model_item::key() const
{
    return key_;
}

DBM_INLINE const kind::tag& model_item::tag() const
{
    return tag_;
}

DBM_INLINE const container& model_item::get_container() const
{
    return *cont_;
}

DBM_INLINE bool model_item::is_primary() const
{
    return primary_.get();
}

DBM_INLINE bool model_item::is_required() const
{
    return required_.get();
}

DBM_INLINE bool model_item::is_taggable() const
{
    return taggable_.get();
}

DBM_INLINE bool model_item::is_null() const
{
    return !cont_ || cont_->is_null();
}

DBM_INLINE bool model_item::is_defined() const
{
    return cont_ && cont_->is_defined();
}

DBM_INLINE const kind::dbtype& model_item::dbtype() const
{
    return dbtype_;
}

DBM_INLINE kind::direction model_item::direction() const
{
    return direction_;
}

DBM_INLINE void model_item::set(const kind::key& v)
{
    key_ = v;
}

DBM_INLINE void model_item::set(const kind::tag& v)
{
    tag_ = v;
}

DBM_INLINE void model_item::set(const kind::primary& v)
{
    primary_ = v;
}

DBM_INLINE void model_item::set(const kind::required& v)
{
    required_ = v;
}

DBM_INLINE void model_item::set(const kind::dbtype& v)
{
    dbtype_ = v;
}

DBM_INLINE void model_item::set(const kind::taggable& v)
{
    taggable_ = v;
}

DBM_INLINE void model_item::set(kind::direction v)
{
    direction_ = v;
}

DBM_INLINE void model_item::set(container_ptr&& v)
{
    cont_ = std::move(v);
}

DBM_INLINE void model_item::set_value(kind::variant& v)
{
    cont_->set(v);
}

DBM_INLINE void model_item::set_value(kind::variant&& v)
{
    set_value(v);
}

DBM_INLINE kind::variant model_item::value() const
{
    return cont_->get();
}

DBM_INLINE std::string model_item::to_string() const
{
    if (!cont_) {
        throw_exception<std::domain_error>("container is null");
    }
    return cont_->to_string();
}

DBM_INLINE void model_item::from_string(std::string_view v)
{
    if (!cont_) {
        make_default_container(); // TODO: make default or throw exception?
    }

    return cont_->from_string(v);
}

DBM_INLINE void model_item::serialize(serializer& s)
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

DBM_INLINE void model_item::deserialize(deserializer& s)
{
    std::string_view sv = !tag_.empty() ? tag_.get() : key_.get();
    if (!cont_) {
        throw_exception<std::domain_error>("Cannot deserialize - no container");
    }
    cont_->deserialize(s, sv);
}

DBM_INLINE void model_item::make_default_container()
{
    cont_ = local<std::string>();
}

}

#endif //DBM_MODEL_ITEM_IPP