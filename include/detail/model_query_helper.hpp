#ifndef DBM_MODEL_QUERY_HELPER_HPP
#define DBM_MODEL_QUERY_HELPER_HPP

#include "model.hpp"

namespace dbm::detail {

struct param_session_type_mysql
{};

struct param_session_type_sqlite
{};

template<class SessionType>
class model_query_helper
{
    static constexpr bool is_SQlite()
    {
        return std::is_same_v<SessionType, param_session_type_sqlite>;
    }

    static constexpr bool is_MySql()
    {
        return std::is_same_v<SessionType, param_session_type_mysql>;
    }

    const model& model_;

public:
    static_assert(is_MySql() || is_SQlite(), "Unsupported session type");

    explicit model_query_helper(const model& m)
        : model_(m)
    {
    }

    std::string write_query() const
    {
        std::string keys, values, duplicate_keys;
        int i = 0, i_duplicate = 0;

        for (const auto& it : model_.items()) {

            if (it.direction() == kind::direction::read_only) {
                continue;
            }

            if (it.is_defined()) {

                if (i) {
                    keys += ",";
                    values += ",";
                }
                if (i_duplicate && !it.is_primary()) {
                    duplicate_keys += ",";
                }

                keys += it.key().get();
                if (it.is_null()) {
                    values += "NULL";
                }
                else {
                    values += "'" + it.to_string() + "'";
                }

                if (!it.is_primary()) {
                    duplicate_keys += it.key().get() + "=VALUES(" + it.key().get() + ")";
                    i_duplicate++;
                }

                ++i;
            }
            else {
                if (it.is_required()) {
                    std::domain_error("Item is required " + it.key().get());
                }
            }
        }

        std::string q = "INSERT ";

        if constexpr (is_SQlite()) {
            if (model_.ignore_insert_) {
                q += "OR IGNORE ";
            }
            else if (model_.update_duplicates_) {
                q += "OR REPLACE ";
            }
        }
        else if constexpr (is_MySql()) {
            if (model_.ignore_insert_) {
                q += "IGNORE ";
            }
        }

        q += "INTO " + model_.table_
             + " (" + keys + ") VALUES (" + values + ")";

        if constexpr (is_MySql()) {
            if (model_.update_duplicates_ && !duplicate_keys.empty()) {
                q += " ON DUPLICATE KEY UPDATE " + duplicate_keys;
            }
        }

        return q;
    }

    std::string read_query(const std::string& extra_condition) const
    {
        std::string q = "SELECT " ;
        std::string what;

        // keys
        int n = 0;
        for (const auto& it : model_.items_) {
            if (it.direction() != kind::direction::write_only) {
                if (n) {
                    what += ",";
                }

                what += it.key().get();
                ++n;
            }
        }

        q += what + " FROM " + model_.table_ + " WHERE ";

        // primary
        n = 0;
        for (const auto& it : model_.items_) {
            // TODO: write only direction
            if (!it.is_primary()) {
                continue;
            }
            if (!it.is_defined()) {
                throw_exception<std::domain_error>("Item " + it.key().get() + " not set");
            }

            if (n) {
                q += " AND ";
            }

            q += it.key().get() + "='" + it.to_string() + "'";
            ++n;
        }

        if (!extra_condition.empty()) {
            q += " AND " + extra_condition;
        }

        return q;
    }

    std::string delete_query() const
    {
        std::string q = "DELETE FROM " + model_.table_ + " WHERE ";

        int n = 0;
        for (const auto& it : model_.items_) {

            if (!it.is_primary()) {
                continue;
            }
            if (!it.is_defined()) {
                std::domain_error("Item " + it.key().get() + " not set");
            }

            if (n) {
                q += " AND ";
            }

            q += it.key().get() + "='" + it.to_string() + "'";
            ++n;
        }

        return q;
    }

    std::string create_table_query(bool if_not_exists) const
    {
        std::string q = "CREATE TABLE ";
        if (if_not_exists) {
            q += "IF NOT EXISTS ";
        }
        q += model_.table_name();

        std::string keys;
        std::string primary_keys;

        for (const auto& it : model_.items()) {

            if (it.is_primary()) {
                if (!primary_keys.empty()) {
                    primary_keys += ", ";
                }
                primary_keys += it.key().get();
            }

            if (!keys.empty()) {
                keys += ", ";
            }
            keys += it.key().get();

            if (!it.dbtype().get().empty()) {
                keys += " " + it.dbtype().get();
            }
        }

        if (!keys.empty()) {
            q += " (" + keys;
            if (!primary_keys.empty()) {
                q += ", PRIMARY KEY (" + primary_keys + ")";
            }
            q += ")";
        }

        return q;
    }

    std::string drop_table_query(bool if_exists) const
    {
        std::string q = "DROP TABLE ";
        if (if_exists) {
            q += "IF EXISTS ";
        }
        q += model_.table_name();
        return q;
    }
};

} // namespace

#endif //DBM_MODEL_QUERY_HELPER_HPP
