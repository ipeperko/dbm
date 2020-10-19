#ifndef DBM_MODEL_IPP
#define DBM_MODEL_IPP

namespace dbm {

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

}

#endif //DBM_MODEL_IPP
