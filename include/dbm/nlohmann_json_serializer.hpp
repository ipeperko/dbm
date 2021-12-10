#ifndef DBM_NLOHMANN_JSON_SERIALIZER_HPP
#define DBM_NLOHMANN_JSON_SERIALIZER_HPP

#include "json.hpp"
#include <dbm/serializer.hpp>

namespace nlohmann {

class deserializer : public dbm::deserializer
{
public:
    explicit deserializer(const json& root)
        : dbm::deserializer()
        , root_(root)
    {}

#define DBM_JSON_DESERIALIZE_TEMPLATE(Type)                                     \
    parse_result deserialize(std::string_view tag, Type& val) const override {  \
        return deserialize_helper<std::decay_t<decltype(val)>>(tag, val);       \
    }

    DBM_DESERIALIZE_GENERIC_FUNC(DBM_JSON_DESERIALIZE_TEMPLATE)

#ifdef DBM_EXPERIMENTAL_BLOB
    parse_result deserialize(std::string_view tag, dbm::kind::blob& val) const override
    {
        std::string s;
        auto res = deserialize_helper(tag, s);
        if (res != ok) {
            return res;
        }

        std::string decoded = dbm::utils::b64_decode(s);
        std::copy(decoded.begin(), decoded.end(), std::back_inserter(val));
        return ok;
    }
#endif

private:
    template<typename T>
    parse_result deserialize_helper(std::string_view tag, T& val) const
    {
        auto it = root_.find(tag.data());
        if (it == root_.end()) {
            return undefined;
        }

        try {
            if (it->is_null()) {
                return null;
            }
            else {
                val = it->get<T>();
                return ok;
            }
        }
        catch (std::exception&)
        {}

        return error;
    }

    json const& root_;
};

class serializer : public dbm::serializer
{
public:
    explicit serializer(json& root)
        : dbm::serializer()
        , root_(root)
        , dser_((root_))
    {}

#define DBM_JSON_SERIALIZE_TEMPLATE(Type)                                       \
    void serialize(std::string_view tag, Type val) override {                   \
        root_[tag.data()] = val;                                                \
    }

    DBM_SERIALIZE_GENERIC_FUNC(DBM_JSON_SERIALIZE_TEMPLATE)

#ifdef DBM_EXPERIMENTAL_BLOB
    void serialize(std::string_view tag, const dbm::kind::blob& blob) override
    {
        root_[tag.data()] = dbm::utils::b64_encode(blob.data(), blob.size());
    }
#endif

private:
    void deserialize(dbm::model& m) override
    {
        dser_ >> m;
    }

    json& root_;
    deserializer dser_;
};

}// namespace nlohmann

#endif//DBM_NLOHMANN_JSON_SERIALIZER_HPP
