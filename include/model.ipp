#ifndef DBM_MODEL_IPP
#define DBM_MODEL_IPP

namespace dbm {

inline const std::string& model::table_name() const
{
    return table_;
}

inline void model::set_table_name(std::string name)
{
    table_ = std::move(name);
}

inline void model::set_sql_duplicates_update(bool t)
{
    update_duplicates_ = t;
}

inline void model::set_sql_ignore_insert(bool t)
{
    ignore_insert_ = t;
}

inline model_item& model::at(const kind::key& key)
{
    auto it = find(key);
    if (it == end()) {
        throw_exception<std::out_of_range>("Item not found '" + key.get() + "'");
    }
    return *it;
}

inline model_item const& model::at(const kind::key& key) const
{
    auto it = find(key);
    if (it == end()) {
        throw_exception<std::out_of_range>("Item not found '" + key.get() + "'");
    }
    return *it;
}

inline model_item& model::at(std::string_view key)
{
    auto it = find(key);
    if (it == end()) {
        throw_exception<std::out_of_range>("Item not found '" + std::string(key) + "'");
    }
    return *it;
}

inline model_item const& model::at(std::string_view key) const
{
    auto it = find(key);
    if (it == end()) {
        throw_exception<std::out_of_range>("Item not found '" + std::string(key) + "'");
    }
    return *it;
}

inline model::iterator model::find(const kind::key& key)
{
    return std::find_if(begin(), end(), [&](const model_item& item) {
        return item.key() == key;
    });
}

inline model::const_iterator model::find(const kind::key& key) const
{
    return std::find_if(begin(), end(), [&](const model_item& item) {
        return item.key() == key;
    });
}

inline model::iterator model::find(std::string_view key)
{
    return std::find_if(begin(), end(), [&](const model_item& item) {
        return item.key().get() == key;
    });
}

inline model::const_iterator model::find(std::string_view key) const
{
    return std::find_if(begin(), end(), [&](const model_item& item) {
        return item.key().get() == key;
    });
}

inline model::item_array& model::items()
{
    return items_;
}

inline model::item_array const& model::items() const
{
    return items_;
}

inline model::iterator model::begin()
{
    return items_.begin();
}

inline model::const_iterator model::begin() const
{
    return items_.begin();
}

inline model::iterator model::end()
{
    return items_.end();
}

inline model::const_iterator model::end() const
{
    return items_.end();
}

inline void model::erase(const kind::key& key)
{
    auto it = find(key);
    if (it == end()) {
        throw_exception<std::out_of_range>("Cannot erase - item '" + key.get() + "' does not exist");
    }
    items_.erase(it);
}

inline void model::erase(std::string_view key)
{
    auto it = find(key);
    if (it == end()) {
        throw_exception<std::out_of_range>("Cannot erase - item '" + std::string(key) + "' does not exist");
    }
    items_.erase(it);
}

inline void model::write_record(session& s)
{
    auto q = s.write_model_query(*this);
    s.query(q);
}

inline void model::read_record(session& s, const std::string& extra_condition)
{
    auto q = s.read_model_query(*this, extra_condition);
    auto rows = s.select(q);

    if (rows.empty()) {
        throw_exception<std::domain_error>("Empty result set" + s.last_statement_info());
    }

    const auto& row = rows.at(0);
    read_record(row);
}

inline void model::read_record(const kind::sql_row& row)
{
    if (row.size() != row.field_names()->size()) {
        throw_exception<std::domain_error>("Row size error"); // TODO: Do we really need this?
    }

    for (auto& it : items_) {
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

inline void model::delete_record(session& s)
{
    auto q = s.delete_model_query(*this);
    s.query(q);
}

inline void model::create_table(session& s, bool if_not_exists)
{
    s.create_table(*this, if_not_exists);
}

inline void model::drop_table(session& s, bool if_exists)
{
    s.drop_table(*this, if_exists);
}

inline void model::serialize(serializer& ser)
{
    for (auto& it : items_) {
        it.serialize(ser);
    }
}

inline void model::deserialize(deserializer& ser)
{
    for (auto& it : items_) {
        it.deserialize(ser);
    }
}

inline model& model::operator>>(serializer& ser)
{
    serialize(ser);
    return *this;
}

inline model& model::operator>>(serializer&& ser)
{
    serialize(ser);
    return *this;
}

inline model& model::operator>>(session& s)
{
    write_record(s);
    return *this;
}

inline model& model::operator<<(const kind::sql_row& row)
{
    read_record(row);
    return *this;
}

}

#endif //DBM_MODEL_IPP
