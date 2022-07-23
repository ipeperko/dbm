#ifndef DBM_MODEL_IPP
#define DBM_MODEL_IPP

namespace dbm {

DBM_INLINE model::model(std::string table)
: table_(std::move(table))
{
}

DBM_INLINE model::model(std::string table, init_list l)
: table_(std::move(table))
, items_(l)
{
}

DBM_INLINE const std::string& model::table_name() const
{
    return table_;
}

DBM_INLINE void model::set_table_name(std::string name)
{
    table_ = std::move(name);
}

DBM_INLINE void model::set_sql_duplicates_update(bool t)
{
    update_duplicates_ = t;
}

DBM_INLINE void model::set_sql_ignore_insert(bool t)
{
    ignore_insert_ = t;
}

DBM_INLINE std::string const& model::table_options() const
{
    return table_opts_;
}

DBM_INLINE void model::set_table_options(std::string opts)
{
    table_opts_ = std::move(opts);
}

DBM_INLINE model_item& model::at(const kind::key& key)
{
    auto it = find(key);
    if (it == end()) {
        throw_exception<std::out_of_range>("Item not found '" + key.get() + "'");
    }
    return *it;
}

DBM_INLINE model_item const& model::at(const kind::key& key) const
{
    auto it = find(key);
    if (it == end()) {
        throw_exception<std::out_of_range>("Item not found '" + key.get() + "'");
    }
    return *it;
}

DBM_INLINE model_item& model::at(std::string_view key)
{
    auto it = find(key);
    if (it == end()) {
        throw_exception<std::out_of_range>("Item not found '" + std::string(key) + "'");
    }
    return *it;
}

DBM_INLINE model_item const& model::at(std::string_view key) const
{
    auto it = find(key);
    if (it == end()) {
        throw_exception<std::out_of_range>("Item not found '" + std::string(key) + "'");
    }
    return *it;
}

DBM_INLINE model_item& model::at(const kind::tag& tag)
{
    auto it = find(tag);
    if (it == end()) {
        throw_exception<std::out_of_range>("Item not found tag '" + tag.get() + "'");
    }
    return *it;
}

DBM_INLINE model_item const& model::at(const kind::tag& tag) const
{
    auto it = find(tag);
    if (it == end()) {
        throw_exception<std::out_of_range>("Item not found tag '" + tag.get() + "'");
    }
    return *it;
}

DBM_INLINE model_item& model::at(std::size_t pos)
{
    return items_.at(pos);
}

DBM_INLINE model_item const& model::at(std::size_t pos) const
{
    return items_.at(pos);
}

DBM_INLINE model::iterator model::find(const kind::key& key)
{
    return std::find_if(begin(), end(), [&](const model_item& item) {
        return item.key() == key;
    });
}

DBM_INLINE model::const_iterator model::find(const kind::key& key) const
{
    return std::find_if(begin(), end(), [&](const model_item& item) {
        return item.key() == key;
    });
}

DBM_INLINE model::iterator model::find(std::string_view key)
{
    return std::find_if(begin(), end(), [&](const model_item& item) {
        return item.key().get() == key;
    });
}

DBM_INLINE model::const_iterator model::find(std::string_view key) const
{
    return std::find_if(begin(), end(), [&](const model_item& item) {
        return item.key().get() == key;
    });
}

DBM_INLINE model::iterator model::find(const kind::tag& tag)
{
    return std::find_if(begin(), end(), [&](const model_item& item) {
        return item.tag() == tag;
    });
}

DBM_INLINE model::const_iterator model::find(const kind::tag& tag) const
{
    return std::find_if(begin(), end(), [&](const model_item& item) {
        return item.tag() == tag;
    });
}


DBM_INLINE model::item_array& model::items()
{
    return items_;
}

DBM_INLINE model::item_array const& model::items() const
{
    return items_;
}

DBM_INLINE model::iterator model::begin()
{
    return items_.begin();
}

DBM_INLINE model::const_iterator model::begin() const
{
    return items_.begin();
}

DBM_INLINE model::iterator model::end()
{
    return items_.end();
}

DBM_INLINE model::const_iterator model::end() const
{
    return items_.end();
}

DBM_INLINE void model::erase(const kind::key& key)
{
    auto it = find(key);
    if (it == end()) {
        throw_exception<std::out_of_range>("Cannot erase - item '" + key.get() + "' does not exist");
    }
    items_.erase(it);
}

DBM_INLINE void model::erase(std::string_view key)
{
    auto it = find(key);
    if (it == end()) {
        throw_exception<std::out_of_range>("Cannot erase - item '" + std::string(key) + "' does not exist");
    }
    items_.erase(it);
}

template<typename DBType>
DBM_INLINE void model::write_record(DBType& s)
{
    auto q = s.write_model_query(*this);
    s.query(q);
}

template<typename DBType>
DBM_INLINE void model::read_record(DBType& s, const std::string& extra_condition)
{
    auto q = s.self().read_model_query(*this, extra_condition);
    auto rows = s.select(q);

    if (rows.empty()) {
        throw_exception<std::domain_error>("Empty result set" + s.last_statement_info());
    }

    const auto& row = rows.at(0);
    read_record(row);
}

DBM_INLINE void model::read_record(const kind::sql_row& row)
{
    if (row.size() != row.field_names()->size()) {
        throw_exception<std::domain_error>("Row size error"); // TODO: Do we really need this?
    }

    for (auto& it : items_) {

        if (it.conf().readable()) {
            std::string const& k = it.key().get();

            if (row.field_map()->find(k) != row.field_map()->end()) {
                const auto& v = row.at(k);

                if (v.null()) {
                    it.set_value(nullptr);
                }
                else {
                    it.from_string(v.get());
                }
            }
        }
    }
}

template<typename DBType>
DBM_INLINE void model::delete_record(DBType& s)
{
    auto q = s.delete_model_query(*this);
    s.query(q);
}

template<typename DBType>
DBM_INLINE void model::create_table(DBType& s, bool if_not_exists)
{
    s.create_table(*this, if_not_exists);
}

template<typename DBType>
DBM_INLINE void model::drop_table(DBType& s, bool if_exists)
{
    s.drop_table(*this, if_exists);
}

DBM_INLINE void model::serialize(serializer& ser)
{
    for (auto& it : items_) {
        it.serialize(ser);
    }
}

template<typename Serializer>
DBM_INLINE void model::serialize2(Serializer& ser)
{
    for (auto& it : items_) {
        if (!it.conf().taggable())
            continue;

        std::string_view tag = !it.tag().empty() ? it.tag().get() : it.key().get();

        if (!it.is_defined()) {
            if (it.conf().required()) {
                throw_exception<std::domain_error>("Serializing failed - item '" + std::string(tag) + "' is required but not defined" );
            }
            continue;
        }

        auto val = it.value();

        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;

            if (it.is_null()) {
                ser.template serialize<std::nullptr_t>(tag, nullptr); // TODO: check if can return nullptr within variant
            }
            else {
                if constexpr(std::is_same_v<T, dbm::kind::detail::timestamp2u_converter>) {
                    // dbm::xml doesn't accept timestamp2u_converter directly,
                    // so we need to specialize the setter
                    ser.template serialize(tag, std::forward<typename T::value_type>(arg.get()));
                }
                else {
                    ser.template serialize(tag, std::forward<T>(arg));
                }
            }
        }, val);
    }
}

template<typename T, typename Serializer>
DBM_INLINE void treat_deserialize_result(Serializer& ser, model_item& item, std::string_view tag)
{
    auto [res, val] = ser.template deserialize<T>(tag);

    switch (res) {
        case dbm::parse_result::ok:
            if (!val) {
                item.get().set_defined(false);
                throw_exception<std::domain_error>("Deserialize failed - optional result without value tag " + std::string(tag));
            }
            item.set_value(val.value());
            break;
//        case dbm::parse_result::undefined:
//            item.get().set_defined(false);
//            break;
        case dbm::parse_result::null:
            item.set_value(nullptr);
            break;
        case dbm::parse_result::undefined:
            // do not change anything
            break;
        case dbm::parse_result::error:
            [[fallthrough]];
        default:
            item.get().set_defined(false);
            throw_exception<std::domain_error>("Deserialize failed - tag " + std::string(tag));
    }
}

template<typename Serializer>
DBM_INLINE void model::deserialize2(Serializer& ser)
{
    for (auto& it : items_) {
        if (!it.conf().taggable())
            continue;

        std::string_view tag = !it.tag().empty() ? it.tag().get() : it.key().get();

        switch(it.get().type()) {
            case kind::data_type::Bool:
                treat_deserialize_result<bool>(ser, it, tag);
                break;
            case kind::data_type::Int32:
                treat_deserialize_result<int32_t>(ser, it, tag);
                break;
            case kind::data_type::Int16:
                treat_deserialize_result<int16_t>(ser, it, tag);
                break;
            case kind::data_type::Int64:
                treat_deserialize_result<int64_t>(ser, it, tag);
                break;
            case kind::data_type::UInt32:
                treat_deserialize_result<uint32_t>(ser, it, tag);
                break;
            case kind::data_type::UInt16:
                treat_deserialize_result<uint16_t>(ser, it, tag);
                break;
            case kind::data_type::UInt64:
                treat_deserialize_result<uint64_t>(ser, it, tag);
                break;
            case kind::data_type::Timestamp2u:
                treat_deserialize_result<kind::detail::timestamp2u_converter::value_type>(ser, it, tag);
                break;
            case kind::data_type::Double:
                treat_deserialize_result<double>(ser, it, tag);
                break;
            case kind::data_type::String:
                treat_deserialize_result<std::string>(ser, it, tag);
                break;
#ifdef DBM_EXPERIMENTAL_BLOB
            case kind::data_type::Blob:
                treat_deserialize_result<blob>(ser, it, tag);
                break;
#endif
            default:
                throw_exception("deserialize from unsupported type " );
        }

    }
}

DBM_INLINE void model::deserialize(deserializer& ser)
{
    for (auto& it : items_) {
        it.deserialize(ser);
    }
}

DBM_INLINE model& model::operator>>(serializer& ser)
{
    serialize(ser);
    return *this;
}

DBM_INLINE model& model::operator>>(serializer&& ser)
{
    serialize(ser);
    return *this;
}

template<typename Serializer>
DBM_INLINE
std::enable_if_t< std::is_base_of_v<serializer_base_tag, Serializer>, model&>
model::operator>>(Serializer& s)
{
    serialize2(s);
    return *this;
}

template<typename Serializer>
DBM_INLINE
std::enable_if_t< std::is_base_of_v<serializer_base_tag, Serializer>, model&>
model::operator>>(Serializer&& s)
{
    serialize2(s);
    return *this;
}

template<typename DBType>
DBM_INLINE
std::enable_if_t< std::is_base_of_v<session_base, DBType>, model&>
model::operator>>(DBType& s)
{
    write_record(s);
    return *this;
}

DBM_INLINE model& model::operator<<(const kind::sql_row& row)
{
    read_record(row);
    return *this;
}

}

#endif //DBM_MODEL_IPP
