#ifndef DBM_XML_SERIALIZER_HPP
#define DBM_XML_SERIALIZER_HPP

#include <dbm/serializer.hpp>
#include <dbm/xml.hpp>
#include <dbm/dbm_common.hpp>

namespace dbm::xml {

template <typename Xml>
class serializer : public dbm::serializer<serializer<Xml>>
{
    Xml& root_;
public:

    static_assert(std::is_same_v<std::decay_t<Xml>, dbm::xml::node>);

    static bool constexpr is_const = std::is_const_v<Xml>;

    explicit serializer(Xml& root)
        : root_(root)
    {}

    auto& get() { return root_; }
    auto const& get() const { return root_; }

    template<typename T, typename Tmp = Xml, typename = std::enable_if_t<not std::is_const_v<Tmp>>>
    void serialize(std::string_view tag, T&& val)
    {
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            auto& item = root_.set(tag, "");
            item.attributes()["xis:nil"] = "true";
        }
        else {
            auto& item = root_.set(tag, std::forward<T>(val));
            auto atit = item.attributes().find("xis:nil");
            if (atit != item.attributes().end())
                item.attributes().erase(atit);
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
            auto val = root_.template get<T>(tag); // TODO: add templated get function
            return { dbm::parse_result::ok, std::move(val) };
        }
        catch (std::exception& e) {
            //deserialization failed
        }

        return { dbm::parse_result::error, std::nullopt };
    }
};

}// namespace dbm::xml

#endif//DBM_XML_SERIALIZER_HPP
