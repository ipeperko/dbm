#ifndef DBM_MODEL_HPP
#define DBM_MODEL_HPP

#include <vector>

namespace dbm {

class session_base;

namespace detail {
template<typename>
class model_query_helper;
}

class model
{
    template<typename> friend class detail::model_query_helper;
public:
    using item_array = std::vector<model_item>;
    using iterator = item_array::iterator;
    using const_iterator = item_array::const_iterator;
    using init_list = std::initializer_list<model_item>;

    model() = default;

    model(const model&) = default;

    model(model&&) = default;

    explicit model(std::string table);

    model(std::string table, init_list l);

    model& operator=(const model&) = default;

    model& operator=(model&&) = default;

    virtual ~model() = default;

    const std::string& table_name() const;

    void set_table_name(std::string name);

    void set_sql_duplicates_update(bool t);

    void set_sql_ignore_insert(bool t);

    std::string const& table_options() const;

    void set_table_options(std::string opts);

    model_item& at(const kind::key& key);

    model_item const& at(const kind::key& key) const;

    model_item& at(std::string_view key);

    model_item const& at(std::string_view key) const;

    model_item& at(const kind::tag& tag);

    model_item const& at(const kind::tag& tag) const;

    model_item& at(std::size_t pos);

    model_item const& at(std::size_t pos) const;

    iterator find(const kind::key& key);

    const_iterator find(const kind::key& key) const;

    iterator find(std::string_view key);

    const_iterator find(std::string_view key) const;

    iterator find(const kind::tag& tag);

    const_iterator find(const kind::tag& tag) const;

    item_array& items();

    item_array const& items() const;

    iterator begin();

    const_iterator begin() const;

    iterator end();

    const_iterator end() const;

    template <typename ... Args>
    model_item& emplace_back(Args&&... args)
    {
        return items_.emplace_back(std::forward<Args>(args)...);
    }

    void erase(const kind::key& key);

    void erase(std::string_view key);

    template<typename DBType>
    void write_record(DBType& s);

    template<typename DBType>
    void read_record(DBType& s, const std::string& extra_condition = "");

    void read_record(const kind::sql_row& row);

    template<typename DBType>
    void delete_record(DBType& s);

    template<typename DBType>
    void create_table(DBType& s, bool if_not_exists=true);

    template<typename DBType>
    void drop_table(DBType& s, bool if_exists=true);

//    void serialize(serializer& ser);

    template<typename Serializer>
    void serialize2(Serializer& ser);

//    void deserialize(deserializer& ser);

    template<typename Serializer>
    void deserialize2(Serializer& ser);

//    model& operator>>(serializer& ser);
//
//    model& operator>>(serializer&& ser);

    template<typename Serializer>
    std::enable_if_t< std::is_base_of_v<serializer_base_tag, Serializer>, model&>
    operator>>(Serializer& s);

    template<typename Serializer>
    std::enable_if_t< std::is_base_of_v<serializer_base_tag, Serializer>, model&>
    operator>>(Serializer&& s);

    template<typename DBType /*, typename = std::enable_if_t< std::is_base_of_v<session_base, DBType>, void>*/>
    std::enable_if_t< std::is_base_of_v<session_base, DBType>, model&>
    operator>>(DBType& s);

    model& operator<<(const kind::sql_row& row);

private:
    std::string table_;
    item_array items_;

    // Config
    bool update_duplicates_{true};
    bool ignore_insert_{false};
    std::string table_opts_; // table options (engine, collations...)
};

}// namespace dbm

#endif//DBM_MODEL_HPP



