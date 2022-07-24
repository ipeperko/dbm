#ifndef DBM_NLOHMANN_JSON_SERIALIZER_HPP
#define DBM_NLOHMANN_JSON_SERIALIZER_HPP

#include "json.hpp"
#include <dbm/serializer.hpp>

namespace nlohmann {

//class deserializer : public dbm::deserializer
//{
//public:
//    explicit deserializer(const json& root)
//        : dbm::deserializer()
//        , root_(root)
//    {}
//
//#define DBM_JSON_DESERIALIZE_TEMPLATE(Type)                                     \
//    parse_result deserialize(std::string_view tag, Type& val) const override {  \
//        return deserialize_helper<std::decay_t<decltype(val)>>(tag, val);       \
//    }
//
//    DBM_DESERIALIZE_GENERIC_FUNC(DBM_JSON_DESERIALIZE_TEMPLATE)
//
//#ifdef DBM_EXPERIMENTAL_BLOB
//    parse_result deserialize(std::string_view tag, dbm::kind::blob& val) const override
//    {
//        std::string s;
//        auto res = deserialize_helper(tag, s);
//        if (res != ok) {
//            return res;
//        }
//
//        std::string decoded = dbm::utils::b64_decode(s);
//        std::copy(decoded.begin(), decoded.end(), std::back_inserter(val));
//        return ok;
//    }
//#endif
//
//private:
//    template<typename T>
//    parse_result deserialize_helper(std::string_view tag, T& val) const
//    {
//        auto it = root_.find(tag.data());
//        if (it == root_.end()) {
//            return undefined;
//        }
//
//        try {
//            if (it->is_null()) {
//                return null;
//            }
//            else {
//                val = it->get<T>();
//                return ok;
//            }
//        }
//        catch (std::exception&)
//        {}
//
//        return error;
//    }
//
//    json const& root_;
//};
//
//class serializer : public dbm::serializer
//{
//public:
//    explicit serializer(json& root)
//        : dbm::serializer()
//        , root_(root)
//        , dser_((root_))
//    {}
//
//    // visible const char* and std::string overloads
//    DBM_USING_SERIALIZE;
//
//#define DBM_JSON_SERIALIZE_TEMPLATE(Type)                                       \
//    void serialize(std::string_view tag, Type val) override {                   \
//        root_[tag.data()] = val;                                                \
//    }
//
//    DBM_SERIALIZE_GENERIC_FUNC(DBM_JSON_SERIALIZE_TEMPLATE)
//
//#ifdef DBM_EXPERIMENTAL_BLOB
//    void serialize(std::string_view tag, const dbm::kind::blob& blob) override
//    {
//        root_[tag.data()] = dbm::utils::b64_encode(blob.data(), blob.size());
//    }
//#endif
//
//private:
//    void deserialize(dbm::model& m) override
//    {
//        dser_ >> m;
//    }
//
//    json& root_;
//    deserializer dser_;
//};

template<typename Json>
class serializer2 : public dbm::serializer2<serializer2<Json>>
{
    Json& root_;
public:

    static_assert(std::is_same_v<std::decay_t<Json>, nlohmann::json>);

    static bool constexpr is_const = std::is_const_v<Json>;

    explicit serializer2(Json& root)
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
