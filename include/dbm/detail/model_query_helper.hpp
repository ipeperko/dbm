#ifndef DBM_MODEL_QUERY_HELPER_HPP
#define DBM_MODEL_QUERY_HELPER_HPP

#include <dbm/model.hpp>

namespace dbm::detail {

struct param_session_type_mysql
{};

struct param_session_type_sqlite
{};

#ifdef DBM_MODEL_QUERY_HELPER_RTTI
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

    virtual model_query_helper_base() = default;

    template <typename UnknownType=void>
    void is_unknown_type() const
    {
        static_assert(! std::is_same_v<UnknownType, UnknownType>, "Unsupported data type");
    }

protected:
    session_type type_ {session_type::unknown};
};
#endif

template<class SessionType>
class model_query_helper
#ifdef DBM_MODEL_QUERY_HELPER_RTTI
    : public model_query_helper_base
#endif
{
    static constexpr bool is_SQlite()
    {
        return std::is_same_v<SessionType, param_session_type_sqlite>;
    }

    static constexpr bool is_MySql()
    {
        return std::is_same_v<SessionType, param_session_type_mysql>;
    }

#ifdef DBM_MODEL_QUERY_HELPER_RTTI
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
#endif

    const model& model_;

public:
    static_assert(is_MySql() || is_SQlite(), "Unsupported session type");

    explicit model_query_helper(const model& m)
        :
#ifdef DBM_MODEL_QUERY_HELPER_RTTI
          model_query_helper_base(session_type_trace()),
#endif
          model_(m)
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
        else if constexpr (is_MySql()) {
            return " AUTO_INCREMENT";
        }
    }

    [[nodiscard]] constexpr std::string_view value_type_string(kind::data_type t) const
    {
        using kind::data_type;

        if constexpr (is_SQlite()) {
            switch (t) {
                case data_type::Bool:
                case data_type::Int:
                case data_type::Short:
                case data_type::Long:
                    return "INTEGER";
                case data_type::Double:
                    return "REAL";
                case data_type::String:
                    return "TEXT";
#if false
                    case data_type::Int_unsigned:
                    return "INTEGER";
                case data_type::Short_unsigned:
                    return "INTEGER";
                case data_type::Long_unsigned:
                    return "INTEGER";
#endif
                case data_type::Nullptr:
                case data_type::Char_ptr:
                case data_type::String_view:
                default:;
            }
        }
        else if constexpr (is_MySql()) {
            switch (t) {
                case data_type::Bool:
                    return "BOOL";
                case data_type::Int:
                    return "INT";
                case data_type::Short:
                    return "SMALLINT";
                case data_type::Long:
                    return "BIGINT";
                case data_type::Double:
                    return "DOUBLE";
                case data_type::String:
                    return "TEXT";
#if false
                    case data_type::Int_unsigned:
                    return "INT UNSIGNED";
                case data_type::Short_unsigned:
                    return "SMALLINT UNSIGNED";
                case data_type::Long_unsigned:
                    return "BIGINT UNSIGNED";
#endif
                case data_type::Nullptr:
                case data_type::Char_ptr:
                case data_type::String_view:
                default:;
            }
        }

        throw_exception<std::domain_error>("Unsupported data type - index " +
                                           std::to_string(static_cast<std::underlying_type_t<kind::data_type>>(t)));
    }
};

template<class SessionType>
DBM_INLINE
std::string model_query_helper<SessionType>::write_query() const
{
    std::string keys, values, duplicate_keys;
    int i = 0;
    [[maybe_unused]] int i_duplicate = 0;

    for (const auto& it : model_.items()) {

        if (!it.conf().writable()) {
            continue;
        }

        if (it.is_defined()) {

            // Commas
            if (i) {
                keys += ",";
                values += ",";
            }

            // Key
            keys += it.key().get();

            // Value
            if (it.is_null()) {
                values += "NULL";
            }
            else {
                if (it.conf().valquotes()) {
                    values += "'";
                }
                values += it.to_string();
                if (it.conf().valquotes()) {
                    values += "'";
                }
            }

            // Duplicate keys
            if constexpr (is_MySql()) {
                if (!it.conf().primary()) {
                    if (i_duplicate) {
                        duplicate_keys += ",";
                    }

                    duplicate_keys += it.key().get() + "=VALUES(" + it.key().get() + ")";
                    ++i_duplicate;
                }
            }

            // Increment counter
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

            // Primary key
            if (it.conf().primary()) {
                if (!primary_keys.empty()) {
                    primary_keys += ", ";
                }
                primary_keys += it.key().get();
            }

            // Key name
            if (!keys.empty()) {
                keys += ", ";
            }
            keys += it.key().get() + " ";

            // Data type
            if (!it.dbtype().get().empty()) {
                // Custom type
                keys += it.dbtype().get();
            }
            else {
                // Standard type
                keys += value_type_string(it.get_container().type());

                // Not null constraint
                if (it.conf().not_null()) {
                    keys += not_null_string().data();
                }

                // Default constraint
                if (it.conf().default_constraint().has_value()) {
                    if (it.conf().default_constraint().is_null()) {
                        keys += " DEFAULT NULL";
                    }
                    else {
                        if (it.get_container().type() == kind::data_type::String) {
                            keys += " DEFAULT '" + it.conf().default_constraint().string_value() + "'";
                        }
                        else {
                            keys += " DEFAULT " + it.conf().default_constraint().string_value();
                        }
                    }
                }

                // Auto increment constraint
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
