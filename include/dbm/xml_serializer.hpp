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
        : b_(std::make_unique<xml::node>("xml"))
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

    parse_result deserialize(std::string_view tag, bool& val) const override
    {
        return deserialize_tmpl<std::decay_t<decltype(val)>>(tag, val);
    }
    parse_result deserialize(std::string_view tag, int& val) const override
    {
        return deserialize_tmpl<std::decay_t<decltype(val)>>(tag, val);
    }
    parse_result deserialize(std::string_view tag, short& val) const override
    {
        return deserialize_tmpl<std::decay_t<decltype(val)>>(tag, val);
    }
    parse_result deserialize(std::string_view tag, long& val) const override
    {
        return deserialize_tmpl<std::decay_t<decltype(val)>>(tag, val);
    }
    parse_result deserialize(std::string_view tag, unsigned int& val) const override
    {
        return deserialize_tmpl<std::decay_t<decltype(val)>>(tag, val);
    }
    parse_result deserialize(std::string_view tag, unsigned short& val) const override
    {
        return deserialize_tmpl<std::decay_t<decltype(val)>>(tag, val);
    }
    parse_result deserialize(std::string_view tag, unsigned long& val) const override
    {
        return deserialize_tmpl<std::decay_t<decltype(val)>>(tag, val);
    }
    parse_result deserialize(std::string_view tag, double& val) const override
    {
        return deserialize_tmpl<std::decay_t<decltype(val)>>(tag, val);
    }
    parse_result deserialize(std::string_view tag, std::string& val) const override
    {
        return deserialize_tmpl<std::decay_t<decltype(val)>>(tag, val);
    }

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
        : b_(std::make_unique<xml::node>("xml"))
        , root_(*b_)
        , dser_(root_)
    {}

    explicit serializer(node& root)
        : root_(root)
        , dser_(root_)
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

    void serialize(std::string_view tag, std::nullptr_t) override
    {
        root_.add(tag, nullptr);
    }

    void serialize(std::string_view tag, bool val) override
    {
        root_.add(tag, val);
    }

    void serialize(std::string_view tag, int val) override
    {
        root_.add(tag, val);
    }

    void serialize(std::string_view tag, short val) override
    {
        root_.add(tag, val);
    }

    void serialize(std::string_view tag, long val) override
    {
        root_.add(tag, val);
    }

    void serialize(std::string_view tag, unsigned int val) override
    {
        root_.add(tag, val);
    }

    void serialize(std::string_view tag, unsigned short val) override
    {
        root_.add(tag, val);
    }

    void serialize(std::string_view tag, unsigned long val) override
    {
        root_.add(tag, val);
    }

    void serialize(std::string_view tag, double val) override
    {
        root_.add(tag, val);
    }

    void serialize(std::string_view tag, const std::string& val) override
    {
        root_.add(tag, val);
    }

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
