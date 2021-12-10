#ifndef DBM_XML_SERIALIZER_HPP
#define DBM_XML_SERIALIZER_HPP

#include <dbm/serializer.hpp>
#include <dbm/xml.hpp>
#include <dbm/dbm_common.hpp>

namespace dbm::xml {

class deserializer : public dbm::deserializer
{
public:

    deserializer()
        : dbm::deserializer()
        , b_(std::make_unique<xml::node>("xml"))
        , root_(*b_)
    {}

    explicit deserializer(node const& root)
        : root_(root)
    {}

    xml::node const& object()
    {
        return root_;
    }

    xml::node const& object() const
    {
        return root_;
    }

#define DBM_XML_DESERIALIZE_TEMPLATE(Type)                                      \
    parse_result deserialize(std::string_view tag, Type& val) const override {  \
        return deserialize_tmpl<std::decay_t<decltype(val)>>(tag, val);         \
    }

    DBM_DESERIALIZE_GENERIC_FUNC(DBM_XML_DESERIALIZE_TEMPLATE)

#ifdef DBM_EXPERIMENTAL_BLOB
    parse_result deserialize(std::string_view tag, dbm::kind::blob& val) const override
    {
        auto it = root_.find(tag);
        if (it == root_.end()) {
            return undefined;
        }

        std::string decoded = dbm::utils::b64_decode(it->value());
        std::copy(decoded.begin(), decoded.end(), std::back_inserter(val));
        return ok;
    }
#endif

private:
    template<typename T>
    parse_result deserialize_tmpl(std::string_view tag, T& val) const
    {
        auto it = root_.find(tag);
        if (it == root_.end()) {
            return undefined;
        }

        try {
            val = root_.get<T>(tag.data());
            return ok;
        }
        catch (std::exception& e) {
            //deserialization failed
        }

        return error;
    }

    std::unique_ptr<xml::node> b_;
    const xml::node& root_;
};

class serializer : public dbm::serializer
{
public:
    serializer()
        : dbm::serializer()
        , b_(std::make_unique<xml::node>("xml"))
        , root_(*b_)
        , dser_(root_)
    {}

    explicit serializer(node& root)
        : root_(root)
        , dser_(root_)
    {}

    void deserialize(dbm::model& m) override
    {
        dser_ >> m;
    }

    template <typename T>
    void set(std::string_view k, const T& val)
    {
        root_.set(k, val);
    }

    xml::node& object()
    {
        return root_;
    }

    xml::node const& object() const
    {
        return root_;
    }

#define DBM_XML_SERIALIZE_TEMPLATE(Type)                                        \
    void serialize(std::string_view tag, Type val) override {                   \
        root_.add(tag, val);                                                    \
    }

    DBM_SERIALIZE_GENERIC_FUNC(DBM_XML_SERIALIZE_TEMPLATE)

#ifdef DBM_EXPERIMENTAL_BLOB
    void serialize(std::string_view tag, const dbm::kind::blob& blob) override
    {
        root_.add(tag, dbm::utils::b64_encode(blob.data(), blob.size()));
    }
#endif

private:
    std::unique_ptr<xml::node> b_;
    xml::node& root_;
    deserializer dser_;
};

}// namespace dbm

#endif//DBM_XML_SERIALIZER_HPP
