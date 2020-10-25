#ifndef DBM_NLOHMANN_JSON_SERIALIZER_HPP
#define DBM_NLOHMANN_JSON_SERIALIZER_HPP

#include "json.hpp"
#include <dbm/serializer.hpp>

namespace nlohmann {

class deserializer : public dbm::deserializer
{
public:
    explicit deserializer(const json& root)
        : root_(root)
    {}

    parse_result deserialize(std::string_view tag, bool& val) const override
    {
        return deserialize_helper<std::decay_t<decltype(val)>>(tag, val);
    }
    parse_result deserialize(std::string_view tag, int& val) const override
    {
        return deserialize_helper<std::decay_t<decltype(val)>>(tag, val);
    }
    parse_result deserialize(std::string_view tag, short& val) const override
    {
        return deserialize_helper<std::decay_t<decltype(val)>>(tag, val);
    }
    parse_result deserialize(std::string_view tag, long& val) const override
    {
        return deserialize_helper<std::decay_t<decltype(val)>>(tag, val);
    }
    parse_result deserialize(std::string_view tag, double& val) const override
    {
        return deserialize_helper<std::decay_t<decltype(val)>>(tag, val);
    }
    parse_result deserialize(std::string_view tag, std::string& val) const override
    {
        return deserialize_helper<std::decay_t<decltype(val)>>(tag, val);
    }

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

template <bool enable_string_to_number = false>
class serializer : public dbm::serializer
{
public:
    explicit serializer(json& root)
        : root_(root)
        , dser_((root_))
    {}

    dbm::model& operator>>(dbm::model& m)
    {
        dser_ >> m;
        return m;
    }

    dbm::model&& operator>>(dbm::model&& m)
    {
        dser_ >> m;
        return std::move(m);
    }

    void serialize(std::string_view tag, std::nullptr_t) override
    {
        root_[tag.data()] = nullptr;
    }

    void serialize(std::string_view tag, bool val) override
    {
        root_[tag.data()] = val;
    }

    void serialize(std::string_view tag, int val) override
    {
        root_[tag.data()] = val;
    }

    void serialize(std::string_view tag, short val) override
    {
        root_[tag.data()] = val;
    }

    void serialize(std::string_view tag, long val) override
    {
        root_[tag.data()] = val;
    }

    void serialize(std::string_view tag, double val) override
    {
        root_[tag.data()] = val;
    }

    void serialize(std::string_view tag, const std::string& val) override
    {
        root_[tag.data()] = val;
    }

#ifdef DBM_EXPERIMENTAL_BLOB
    void serialize(std::string_view tag, const dbm::kind::blob& blob) override
    {
        root_[tag.data()] = dbm::utils::b64_encode(blob.data(), blob.size());
    }
#endif

private:
    json& root_;
    deserializer dser_;
};

}// namespace nlohmann

#endif//DBM_NLOHMANN_JSON_SERIALIZER_HPP
