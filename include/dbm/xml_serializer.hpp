#ifndef DBM_XML_SERIALIZER_HPP
#define DBM_XML_SERIALIZER_HPP

#include <dbm/serializer.hpp>
#include <dbm/xml.hpp>
#include <dbm/dbm_common.hpp>

namespace dbm::xml {

//class deserializer : public dbm::deserializer
//{
//public:
//    // local xml object will be created
//    deserializer()
//        : dbm::deserializer()
//        , buffer_(std::make_unique<xml::node>("xml"))
//        , root_(*buffer_)
//    {}
//
//    // using external xml object
//    explicit deserializer(node const& root)
//        : root_(root)
//    {}
//
//    // get object (makes sense only if used with local xml object)
//    xml::node const& object()
//    {
//        return root_;
//    }
//
//    // const version
//    xml::node const& object() const
//    {
//        return root_;
//    }
//
//#define DBM_XML_DESERIALIZE_TEMPLATE(Type)                                      \
//    parse_result deserialize(std::string_view tag, Type& val) const override {  \
//        return deserialize_tmpl<std::decay_t<decltype(val)>>(tag, val);         \
//    }
//
//    DBM_DESERIALIZE_GENERIC_FUNC(DBM_XML_DESERIALIZE_TEMPLATE)
//
//#ifdef DBM_EXPERIMENTAL_BLOB
//    parse_result deserialize(std::string_view tag, dbm::kind::blob& val) const override
//    {
//        auto it = root_.find(tag);
//        if (it == root_.end()) {
//            return undefined;
//        }
//
//        std::string decoded = dbm::utils::b64_decode(it->value());
//        std::copy(decoded.begin(), decoded.end(), std::back_inserter(val));
//        return ok;
//    }
//#endif
//
//private:
//    template<typename T>
//    parse_result deserialize_tmpl(std::string_view tag, T& val) const
//    {
//        auto it = root_.find(tag);
//        if (it == root_.end()) {
//            return undefined;
//        }
//
//        try {
//            val = root_.get<T>(tag.data());
//            return ok;
//        }
//        catch (std::exception& e) {
//            //deserialization failed
//        }
//
//        return error;
//    }
//
//    std::unique_ptr<xml::node> buffer_;
//    const xml::node& root_;
//};
//
//class serializer : public dbm::serializer
//{
//public:
//    // local xml object will be created
//    serializer()
//        : dbm::serializer()
//        , buffer_(std::make_unique<xml::node>("xml"))
//        , root_(*buffer_)
//        , dser_(root_)
//    {}
//
//    // using external xml object
//    explicit serializer(node& root)
//        : root_(root)
//        , dser_(root_)
//    {}
//
//    // get object (makes sense only if used with local xml object)
//    xml::node& object()
//    {
//        return root_;
//    }
//
//    // const version
//    xml::node const& object() const
//    {
//        return root_;
//    }
//
//#define DBM_XML_SERIALIZE_TEMPLATE(Type)                                        \
//    void serialize(std::string_view tag, Type val) override {                   \
//        root_.set(tag, val);                                                    \
//    }
//
//    DBM_SERIALIZE_GENERIC_FUNC_ALL_STRING(DBM_XML_SERIALIZE_TEMPLATE)
//
//#ifdef DBM_EXPERIMENTAL_BLOB
//    void serialize(std::string_view tag, const dbm::kind::blob& blob) override
//    {
//        root_.set(tag, dbm::utils::b64_encode(blob.data(), blob.size()));
//    }
//#endif
//
//private:
//    // method is required for deserializing via serializer
//    void deserialize(dbm::model& m) override
//    {
//        dser_ >> m;
//    }
//
//    std::unique_ptr<xml::node> buffer_;
//    xml::node& root_;
//    deserializer dser_;
//};


class serializer2 : public dbm::serializer2<serializer2>
{
public:
    explicit serializer2(dbm::xml::node& root)
        : root_(root)
    {}

    template<typename T>
    void serialize(std::string_view tag, T&& val)
    {
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            root_.set(tag, "");
            root_.find(tag)->attributes()["xis:nil"] = "true";
        }
        else {
            root_.set(tag, std::forward<T>(val));
        }
    }

    template<typename T>
    std::pair<dbm::parse_result, std::optional<T>> deserialize(std::string_view tag) const
    {
        auto it = root_.find(tag);
        if (it == root_.end())
            return { dbm::parse_result::undefined, std::nullopt };

        auto nil = it->attributes().find("xis:nil");
        if (nil != it->attributes().end() && nil->second == "true")
            return { dbm::parse_result::null, std::nullopt };

        try {
            auto val = root_.get<T>(tag); // TODO: add templated get function
            return { dbm::parse_result::ok, std::move(val) };
        }
        catch (std::exception& e) {
            //deserialization failed
        }

        return { dbm::parse_result::error, std::nullopt };
    }

    dbm::xml::node& root_;
};

}// namespace dbm::xml

#endif//DBM_XML_SERIALIZER_HPP
