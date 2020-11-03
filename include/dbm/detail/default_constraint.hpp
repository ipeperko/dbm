#ifndef DBM_DEFAULT_CONSTRAINT_HPP
#define DBM_DEFAULT_CONSTRAINT_HPP

#include <optional>
#include <variant>

namespace dbm::kind::detail {

class default_constraint
{
public:
    using val_t = std::optional<std::variant<std::nullptr_t, std::string>>;

    default_constraint() = default;

    explicit default_constraint(std::string&& val) noexcept
        : val_(std::move(val))
    {
    }

    template <typename T>
    explicit default_constraint(T const& val)
    {
        if constexpr (std::is_same_v<T, std::nullopt_t>) {
            // do nothing, already nullopt
        }
        else if constexpr (std::is_same_v<T, std::nullptr_t>) {
            val_ = nullptr;
        }
        else if constexpr (utils::is_string_type<T>::value) {
            val_ = std::string(val);
        }
        else {
            val_ = std::to_string(val);
        }
    }

    ~default_constraint() = default;

    [[nodiscard]] bool has_value() const noexcept
    {
        return val_.has_value();
    }

    [[nodiscard]] bool is_null() const
    {
        if (!has_value()) {
            throw_exception<std::domain_error>("default_constraint is_null can only be called if object has value");
        }
        return std::holds_alternative<std::nullptr_t>(val_.value());
    }

    [[nodiscard]] std::string string_value() const
    {
        if (!has_value() || std::holds_alternative<std::nullptr_t>(val_.value())) {
            throw_exception<std::domain_error>("default_constraint string_value can only be called if object has value and value not nullptr");
        }
        return std::get<std::string>(val_.value());
    }

private:
    val_t val_;
};

}

#endif //DBM_DEFAULT_CONSTRAINT_HPP
