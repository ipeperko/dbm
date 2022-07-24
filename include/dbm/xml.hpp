#ifndef DBM_XML_HPP
#define DBM_XML_HPP

#include <dbm/dbm_common.hpp>
#include <vector>
#include <list>
#include <sstream>
#include <unordered_map>

namespace dbm::xml {

/*
 * -------------------------------------------------------------------
 * Type definitions
 * -------------------------------------------------------------------
 */

typedef std::unordered_map<std::string, std::string> attribute_map;

#if __cplusplus >= 201703L
typedef std::string_view string_view;
#else
typedef const std::string& string_view;
#endif

/*
 * -------------------------------------------------------------------
 * Utils
 * -------------------------------------------------------------------
 */

namespace utils {

template<typename T>
struct is_string_type : std::integral_constant<bool, std::is_constructible<std::string, T>::value>
{
};

}// namespace utils

/*
 * -------------------------------------------------------------------
 * Xml Node
 * -------------------------------------------------------------------
 */

class node
{
public:
    typedef std::list<node> Items;
    typedef Items::iterator iterator;
    typedef Items::const_iterator const_iterator;

    static constexpr const char* stdXMLDeclaration = R"(<?xml version="1.0" encoding="utf-8"?>)";

    // Constructors
    DBM_EXPORT node() = default;

    DBM_EXPORT node(string_view tag, string_view value = "");

    DBM_EXPORT node(const node& oth);

    DBM_EXPORT node(node&& oth);

#if __cplusplus >= 201402L
    template<typename T, typename = std::enable_if_t<std::is_floating_point<T>::value or std::is_integral<T>::value>>
    DBM_EXPORT node(const std::string& tag, T&& val)
        : tag_(tag)
        , parent_{nullptr}
    {
        std::ostringstream ss;
        ss << val;
        value_ = ss.str();
    }
#endif

    // Deconstructor
    DBM_EXPORT ~node();

    // Assignments
    DBM_EXPORT node& operator=(const node& oth);

    DBM_EXPORT node& operator=(node&& oth);

    // Cleanup
    DBM_EXPORT void cleanup();

    // Adding elements
    DBM_EXPORT node& add(string_view name, std::string&& param);

    DBM_EXPORT node& add(string_view name = "");

    DBM_EXPORT node& add(const node& oth);

    DBM_EXPORT node& add(node&& oth);

    template<typename T>
    node& add(string_view name, T&& val)
    {
        items_.emplace_back(name);
        node& n = items_.back();
        n.set_value(std::forward<T>(val));
        n.set_parent(this);
        return n;
    }

    DBM_EXPORT node& add(string_view name, std::nullptr_t);

    // Get/set tag, value, attriute access
    DBM_EXPORT std::string& tag() { return tag_; }

    DBM_EXPORT const std::string& tag() const { return tag_; }

    DBM_EXPORT void set_tag(std::string&& str) { tag_ = std::move(str); }

    DBM_EXPORT std::string& value() { return value_; }

    DBM_EXPORT const std::string& value() const { return value_; }

    DBM_EXPORT void set_value(string_view str) { value_ = str; }

    DBM_EXPORT void set_value(const char* str) { value_ = str; }

    DBM_EXPORT void set_value(std::string&& str) { value_ = std::move(str); }

    DBM_EXPORT void set_value(std::nullptr_t) { set_value(std::string{}); }

    template<typename T>
    typename std::enable_if_t<not utils::is_string_type<T>::value, void>
    set_value(T&& val)
    {
        std::ostringstream ss;
        ss << val;
        value_ = ss.str();
    }

    template<typename T>
    node& set(string_view key, T&& val)
    {
        auto n = find(key);
        if (n == end()) {
            return add(key, std::forward<T>(val));
        }
        else {
            n->set_value(std::forward<T>(val));
            return *n;
        }
    }

    DBM_EXPORT node& set(string_view key, std::nullptr_t)
    {
        auto n = find(key);
        if (n == end()) {
            return add(key, nullptr);
        }
        else {
            n->set_value(nullptr);
            return *n;
        }
    }

    DBM_EXPORT attribute_map& attributes();

    DBM_EXPORT const attribute_map& attributes() const;

    // Find by tag
    DBM_EXPORT iterator find(string_view tag);

    DBM_EXPORT const_iterator find(string_view tag) const;

    DBM_EXPORT node* find_node_recursive(string_view tag);

    DBM_EXPORT node const* find_node_recursive(string_view tag) const;

    // Find multiple items with same tag name
    DBM_EXPORT std::vector<iterator> find_nodes(string_view key);

    DBM_EXPORT std::vector<const_iterator> find_nodes(string_view key) const;

    // Value getters

    // @return type: T
    template<typename T>
    typename std::enable_if<not utils::is_string_type<T>::value, T>::type
    get(string_view key) const
    {
        T val;
        auto n = find(key);
        if (n == end()) {
            throw std::out_of_range("Cannot find tag " + std::string(key));
        }
        std::istringstream ss((n)->value());
        if (!(ss >> val)) {
            throw std::domain_error("Conversion failed tag: " + std::string(key));
        };
        return val;
    }

    // @return type: std::string
    template<typename T>
    typename std::enable_if<utils::is_string_type<T>::value, std::string>::type
    get(string_view key) const
    {
        auto n = find(key);
        if (n == end()) {
            throw std::out_of_range("Cannot find tag " + std::string(key));
        }
        return n->value();
    }

    // @return type: T
    template<typename T>
    typename std::enable_if<not utils::is_string_type<T>::value, T>::type
    get_optional(string_view key, T opt) const
    {
        auto n = find(key);
        if (n != end()) {
            std::istringstream ss(n->value());
            T val;
            if (ss >> val) {
                return val;
            }
            else {
                return opt;
            }
        }
        else {
            return opt;
        }
    }

    // @return type: std::string
    template<typename T>
    typename std::enable_if<utils::is_string_type<T>::value, std::string>::type
    get_optional(string_view key, const std::string& opt = "") const
    {
        auto n = find(key);
        if (n != end()) {
            return n->value();
        }
        return opt;
    }

    // Element access
    DBM_EXPORT node& at(size_t idx);

    DBM_EXPORT const node& at(size_t idx) const;

    DBM_EXPORT node& front();

    DBM_EXPORT const node& front() const;

    DBM_EXPORT node& back();

    DBM_EXPORT const node& back() const;

    // Iterators
    DBM_EXPORT iterator begin() noexcept;

    DBM_EXPORT const_iterator begin() const noexcept;

    DBM_EXPORT iterator end() noexcept;

    DBM_EXPORT const_iterator end() const noexcept;

    // Children, parent etc
    DBM_EXPORT Items& items() noexcept;

    DBM_EXPORT const Items& items() const noexcept;

    DBM_EXPORT node* parent() noexcept;

    DBM_EXPORT const node* parent() const noexcept;

    DBM_EXPORT bool is_root() const noexcept;

    DBM_EXPORT int level() const noexcept;

    // Parser
    DBM_EXPORT static node parse(string_view data);

    DBM_EXPORT static node parse(std::istream& is);

    // Serialize
    DBM_EXPORT std::string to_string(unsigned ident = 0) const;

    DBM_EXPORT std::string to_string(bool declr, unsigned ident) const;

protected:
    std::string to_string_helper(bool declr, int level, unsigned ident) const;
    void set_parent(node* parent) { parent_ = parent; }

    std::string tag_;
    std::string value_;
    node* parent_{nullptr};
    Items items_;
    attribute_map attrs_;
};


/*
 * -------------------------------------------------------------------
 * Bindings
 * -------------------------------------------------------------------
 */

class binding_base
{
public:
    explicit binding_base(string_view key)
        : key_(key)
    {}

    virtual ~binding_base() = default;

    virtual void deserialize(const node& n) = 0;

    virtual node serialize() const = 0;

    std::string const& key()
    {
        return key_;
    }

protected:
    std::string key_;
};


// Binding template
template<typename T>
class binding : public binding_base
{
    T& x;
public:
    explicit binding(string_view key, T& x)
        : binding_base(key)
        , x(x)
    {}

    void deserialize(const node& n) override
    {
        std::istringstream is(n.value());
        if (!(is >> x)) {
            throw std::runtime_error("Conversion failed - " + key_ + ":" + n.value());
        }
    }

    node serialize() const override
    {
        std::ostringstream ss;
        ss << x;
        return { key_, ss.str()};
    }
};

// std::string specialization
template<>
class binding<std::string> : public binding_base
{
    std::string& x;

public:
    explicit binding(string_view key, std::string& x)
        : binding_base(key)
        , x(x)
    {}

    void deserialize(const node& n) override
    {
        x = n.value();
    }

    node serialize() const override
    {
        return { key_, x};
    }
};

// Enum binding
template<typename Enum, typename EnumBase>
class binding_enum : public binding_base
{
    Enum& x;

public:
    explicit binding_enum(string_view key, Enum& x)
        : binding_base(key)
        , x(x)
    {}

    void deserialize(const node& n) override
    {
        std::istringstream is(n.value());
        EnumBase xx;

        if (!(is >> xx)) {
            throw std::runtime_error("Conversion failed - " + key_ + ":" + n.value());
        }

        x = static_cast<Enum>(xx);
    }

    node serialize() const override
    {
        std::ostringstream ss;
        EnumBase xx = static_cast<EnumBase>(x);
        ss << xx;
        return { key_, ss.str()};
    }
};

// Function binding
template<typename FNfromXml, typename FNtoXml>
class binding_func : public binding_base
{
    FNfromXml from_xml;
    FNtoXml to_xml;

public:
    explicit binding_func(string_view key, FNfromXml from_xml, FNtoXml to_xml)
        : binding_base(key)
        , from_xml(from_xml)
        , to_xml(to_xml)
    {}
    ~binding_func() = default;

    void deserialize(const node& n) override
    {
        from_xml(n);
    }

    node serialize() const override
    {
        return to_xml();
    }
};

class model : public std::vector<binding_base*>
{
public:
    model() {}

    ~model()
    {
        for (auto& it : *this) {
            delete it;
        }
    }

    template<typename T>
    void add_binding(string_view key, T& ref)
    {
        push_back(new binding<T>(key, ref));
    }

    template<typename TEnum, typename TEnumBase>
    void add_binding_enum(string_view key, TEnum& ref)
    {
        push_back(new binding_enum<TEnum, TEnumBase>(key, ref));
    }

    template<typename FNfromXml = void(const node&), typename FNtoXml = node(void)>
    void add_binding_func(string_view key, FNfromXml from_xml, FNtoXml to_xml)
    {
        push_back(new binding_func<FNfromXml, FNtoXml>(key, from_xml, to_xml));
    }

    void from_xml(const node& root)
    {
        for (auto it = root.begin(); it != root.end(); it++) {
            auto binding = find_by_key(it->tag());
            if (binding != end()) {
                (*binding)->deserialize(*it);
            }
        }
    }

    void to_xml(node& root)
    {
        for (const auto& it : *this) {
            root.add() = it->serialize();
        }
    }

    iterator find_by_key(string_view key)
    {
        for (auto it = begin(); it != end(); ++it) {
            if ((*it)->key() == key) {
                return it;
            }
        }
        return end();
    }
};

}// namespace

#endif//DBM_XML_HPP