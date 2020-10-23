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

DBM_INLINE void model::write_record(session& s)
{
    auto q = s.write_model_query(*this);
    s.query(q);
}

DBM_INLINE void model::read_record(session& s, const std::string& extra_condition)
{
    auto q = s.read_model_query(*this, extra_condition);
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

DBM_INLINE void model::delete_record(session& s)
{
    auto q = s.delete_model_query(*this);
    s.query(q);
}

DBM_INLINE void model::create_table(session& s, bool if_not_exists)
{
    s.create_table(*this, if_not_exists);
}

DBM_INLINE void model::drop_table(session& s, bool if_exists)
{
    s.drop_table(*this, if_exists);
}

DBM_INLINE void model::serialize(serializer& ser)
{
    for (auto& it : items_) {
        it.serialize(ser);
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

DBM_INLINE model& model::operator>>(session& s)
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
