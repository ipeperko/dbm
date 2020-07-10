#include "model.hpp"

namespace dbm {

void model::write_record(session& s)
{
    auto q = s.write_model_query(*this);
    s.query(q);
}

void model::read_record(session& s, const std::string& extra_condition)
{
    auto q = s.read_model_query(*this, extra_condition);
    auto rows = s.select(q);

    if (rows.empty()) {
        throw_exception<std::domain_error>("Empty result set" + s.last_statement_info());
    }

    const auto& row = rows.at(0);
    read_record(row);
}

void model::read_record(const kind::sql_row& row)
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

void model::delete_record(session& s)
{
    auto q = s.delete_model_query(*this);
    s.query(q);
}

}

