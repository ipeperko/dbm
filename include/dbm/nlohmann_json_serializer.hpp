#ifndef DBM_NLOHMANN_JSON_SERIALIZER_HPP
#define DBM_NLOHMANN_JSON_SERIALIZER_HPP

#include "json.hpp"
#include <dbm/serializer.hpp>

namespace nlohmann {

template<typename Json>
class serializer : public dbm::serializer<serializer<Json>>
{
    Json& root_;
public:

    static_assert(std::is_same_v<std::decay_t<Json>, nlohmann::json>);

    static bool constexpr is_const = std::is_const_v<Json>;

    explicit serializer(Json& root)
        : root_(root)
    {}

    auto& get() { return root_; }
    auto const& get() const { return root_; }

    template<typename T, typename Tmp = Json, typename = std::enable_if_t<not std::is_const_v<Tmp>>>
    void serialize(std::string_view tag, T&& val)
    {
        if constexpr (std::is_same_v<T, std::string_view>) {
            root_[tag.data()] = val.data();
        }
        else {
            root_[tag.data()] = std::forward<T>(val);
        }
    }

    template<typename T>
    std::pair<dbm::parse_result, std::optional<T>> deserialize(std::string_view tag) const
    {
        auto it = root_.find(tag);
        if (it == root_.end())
            return { dbm::parse_result::undefined, std::nullopt };

        if (it->is_null())
            return { dbm::parse_result::null, std::nullopt };

        try {
            auto val = it.value().template get<T>();
            return { dbm::parse_result::ok, std::move(val) };
        }
        catch (std::exception& e) {
            //deserialization failed
        }

        return { dbm::parse_result::error, std::nullopt };
    }
};

}// namespace nlohmann

#endif//DBM_NLOHMANN_JSON_SERIALIZER_HPP
