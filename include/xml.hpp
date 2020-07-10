#ifndef DBM_XML_HPP
#define DBM_XML_HPP

#include <vector>
#include <list>
#include <sstream>
#include <unordered_map>

namespace dbm {
namespace xml {

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
    node() = default;

    node(string_view tag, string_view value = "");

    node(const node& oth);

    node(node&& oth);

#if __cplusplus >= 201402L
    template<typename T, typename = std::enable_if_t<std::is_floating_point<T>::value or std::is_integral<T>::value>>
    node(const std::string& tag, T&& val)
        : tag_(tag)
        , parent_{nullptr}
    {
        std::ostringstream ss;
        ss << val;
        value_ = ss.str();
    }
#endif

    // Deconstructor
    ~node();

    // Assignments
    node& operator=(const node& oth);

    node& operator=(node&& oth);

    // Cleanup
    void cleanup();

    // Adding elements
    node& add(string_view name, string_view param);

    node& add(string_view name = "");

    node& add(const node& oth);

    node& add(node&& oth);

    template<typename T>
    node& add(string_view name, T val)
    {
        std::ostringstream ss;
        ss << val;
        items_.emplace_back(name, ss.str());
        node& n = items_.back();
        n.set_parent(this);
        return n;
    }

    node& add(string_view name, std::nullptr_t);

    // Get/set tag, value, attriute access
    std::string& tag();

    const std::string& tag() const;

    void set_tag(string_view str);

    std::string& value();

    const std::string& value() const;

    void set_value(string_view str);

    template<typename T>
    typename std::enable_if_t<not utils::is_string_type<T>::value, void>
    set_value(const T& val)
    {
        std::ostringstream ss;
        ss << val;
        value_ = ss.str();
    }

    template<typename T>
    void set(string_view key, const T& val)
    {
        auto n = find(key);
        if (n == end()) {
            add(key, val);
        }
        else {
            n->set_value(val);
        }
    }

    attribute_map& attributes();

    const attribute_map& attributes() const;

    // Find by tag
    iterator find(string_view tag);

    const_iterator find(string_view tag) const;

    node* find_node_recursive(string_view tag);

    node const* find_node_recursive(string_view tag) const;

    // Find multiple items with same tag name
    std::vector<iterator> find_nodes(string_view key);

    std::vector<const_iterator> find_nodes(string_view key) const;

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
    node& at(size_t idx);

    const node& at(size_t idx) const;

    node& front();

    const node& front() const;

    node& back();

    const node& back() const;

    // Iterators
    iterator begin();

    const_iterator begin() const;

    iterator end();

    const_iterator end() const;

    // Children, parent etc
    Items& items();

    const Items& items() const;

    node* parent();

    const node* parent() const;

    bool is_root() const;

    int level() const;

    // Parser
    static node parse(string_view data);

    static node parse(std::istream& is);

    // Serialize
    std::string to_string(unsigned ident = 0) const;

    std::string to_string(bool declr, unsigned ident) const;

protected:
    void set_parent(node* parent) { parent_ = parent; }

    std::string tag_;
    std::string value_;
    node* parent_{nullptr};
    Items items_;
    attribute_map attrs_;
};

}// namespace xml
}// namespace dbm

#endif//DBM_XML_HPP