#ifndef DBM_MODEL_QUERY_HELPER_HPP
#define DBM_MODEL_QUERY_HELPER_HPP

#include <dbm/model.hpp>

namespace dbm::detail {

struct param_session_type_mysql
{};

struct param_session_type_sqlite
{};

class model_query_helper_base
{
public:

    enum class session_type
    {
        unknown,
        MySQL,
        SQLite
    };

    model_query_helper_base() = delete;

    explicit model_query_helper_base(session_type t)
        : type_(t)
    {}

    template <typename UnknownType=void>
    void is_unknown_type() const
    {
        static_assert(! std::is_same_v<UnknownType, UnknownType>, "Unsupported data type");
    }

    template<typename T>
    std::string value_type_string() const
    {
        if (type_ == session_type::MySQL) {
            if constexpr (std::is_same_v<T, bool>) {
                return "BOOL";
            }
            else if constexpr (std::is_same_v<T, int>) {
                return "INT";
            }
            else if constexpr (std::is_same_v<T, unsigned int>) {
                return "INT UNSIGNED";
            }
            else if constexpr (std::is_same_v<T, short>) {
                return "SMALLINT";
            }
            else if constexpr (std::is_same_v<T, unsigned short>) {
                return "SMALLINT UNSIGNED";
            }
            else if constexpr (std::is_same_v<T, long>) {
                return "BIGINT";
            }
            else if constexpr (std::is_same_v<T, unsigned long>) {
                return "BIGINT UNSIGNED";
            }
            else if constexpr (std::is_same_v<T, double>) {
                return "DOUBLE";
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                return "TEXT";
            }
            else {
                model_query_helper_base::is_unknown_type();
            }
        }
        else if (type_ == session_type::SQLite) {
            if constexpr (std::is_same_v<T, bool>) {
                return "INTEGER";
            }
            else if constexpr (std::is_same_v<T, int>) {
                return "INTEGER";
            }
            else if constexpr (std::is_same_v<T, unsigned int>) {
                return "INTEGER";
            }
            else if constexpr (std::is_same_v<T, short>) {
                return "INTEGER";
            }
            else if constexpr (std::is_same_v<T, unsigned short>) {
                return "INTEGER";
            }
            else if constexpr (std::is_same_v<T, long>) {
                return "INTEGER";
            }
            else if constexpr (std::is_same_v<T, unsigned long>) {
                return "INTEGER";
            }
            else if constexpr (std::is_same_v<T, double>) {
                return "REAL";
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                return "TEXT";
            }
            else {
                model_query_helper_base::is_unknown_type();
            }
        }

        // Shouldn't be here - just to disable compiler warning
        return "";
    }

protected:
    session_type type_ {session_type::unknown};
};

template<class SessionType>
class model_query_helper : public model_query_helper_base
{
    static constexpr bool is_SQlite()
    {
        return std::is_same_v<SessionType, param_session_type_sqlite>;
    }

    static constexpr bool is_MySql()
    {
        return std::is_same_v<SessionType, param_session_type_mysql>;
    }

    static constexpr session_type session_type_trace()
    {
        if constexpr (std::is_same_v<SessionType, param_session_type_mysql>) {
            return session_type::MySQL;
        }
        else if constexpr (std::is_same_v<SessionType, param_session_type_sqlite>) {
            return session_type::SQLite;
        }
        else {
            return session_type::unknown;
        }
    }

    const model& model_;

public:
    static_assert(is_MySql() || is_SQlite(), "Unsupported session type");

    explicit model_query_helper(const model& m)
        : model_query_helper_base(session_type_trace())
        , model_(m)
    {
    }

    [[nodiscard]] std::string write_query() const;

    [[nodiscard]] std::string read_query(const std::string& extra_condition) const;

    [[nodiscard]] std::string delete_query() const;

    [[nodiscard]] std::string create_table_query(bool if_not_exists) const;

    [[nodiscard]] std::string drop_table_query(bool if_exists) const;

    [[nodiscard]] constexpr std::string_view not_null_string() const
    {
        return " NOT NULL";
    }

    [[nodiscard]] constexpr std::string_view auto_increment_string() const
    {
        if constexpr (is_SQlite()) {
            return "";
        }
        if constexpr (is_MySql()) {
            return " AUTO_INCREMENT";
        }
    }
};

template<class SessionType>
DBM_INLINE
std::string model_query_helper<SessionType>::write_query() const
{
    std::string keys, values, duplicate_keys;
    int i = 0, i_duplicate = 0;

    for (const auto& it : model_.items()) {

        if (!it.conf().writable()) {
            continue;
        }

        if (it.is_defined()) {

            if (i) {
                keys += ",";
                values += ",";
            }
            if (i_duplicate && !it.conf().primary()) {
                duplicate_keys += ",";
            }

            keys += it.key().get();
            if (it.is_null()) {
                values += "NULL";
            }
            else {
                values += "'" + it.to_string() + "'";
            }

            if (!it.conf().primary()) {
                duplicate_keys += it.key().get() + "=VALUES(" + it.key().get() + ")";
                i_duplicate++;
            }

            ++i;
        }
        else {
            if (it.conf().required()) {
                throw_exception<std::domain_error>("Item is required " + it.key().get());
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

template<class SessionType>
DBM_INLINE
std::string model_query_helper<SessionType>::read_query(const std::string& extra_condition) const
{
    std::string q = "SELECT " ;
    std::string what;

    // keys
    int n = 0;
    for (const auto& it : model_.items_) {
        if (it.conf().readable()) {
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
        if (!it.conf().primary()) {
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

template<class SessionType>
DBM_INLINE
std::string model_query_helper<SessionType>::delete_query() const
{
    std::string q = "DELETE FROM " + model_.table_ + " WHERE ";

    int n = 0;
    for (const auto& it : model_.items_) {

        if (!it.conf().primary()) {
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

    return q;
}

template<class SessionType>
DBM_INLINE
std::string model_query_helper<SessionType>::create_table_query(bool if_not_exists) const
{
    std::string q = "CREATE TABLE ";
    if (if_not_exists) {
        q += "IF NOT EXISTS ";
    }
    q += model_.table_name();

    std::string keys;
    std::string primary_keys;

    for (const auto& it : model_.items()) {

        if (it.conf().creatable()) {

            if (it.conf().primary()) {
                if (!primary_keys.empty()) {
                    primary_keys += ", ";
                }
                primary_keys += it.key().get();
            }

            if (!keys.empty()) {
                keys += ", ";
            }

            keys += it.key().get() + " ";

            if (!it.dbtype().get().empty()) {
                keys += it.dbtype().get();
            }
            else {
                keys += it.get_container().type_to_string(this);

                if (it.conf().not_null()) {
                    keys += not_null_string().data();
                }

                if (it.conf().auto_increment()) {
                    keys += auto_increment_string().data();
                }
            }
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

template<class SessionType>
DBM_INLINE
std::string model_query_helper<SessionType>::drop_table_query(bool if_exists) const
{
    std::string q = "DROP TABLE ";
    if (if_exists) {
        q += "IF EXISTS ";
    }
    q += model_.table_name();
    return q;
}

} // namespace

#endif //DBM_MODEL_QUERY_HELPER_HPP
