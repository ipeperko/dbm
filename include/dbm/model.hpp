#ifndef DBM_MODEL_HPP
#define DBM_MODEL_HPP

#include <dbm/model_item.hpp>
#include <dbm/model_item.ipp>
#include <vector>

namespace dbm {

class session;

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

    model_item& at(const kind::key& key);

    model_item const& at(const kind::key& key) const;

    model_item& at(std::string_view key);

    model_item const& at(std::string_view key) const;

    iterator find(const kind::key& key);

    const_iterator find(const kind::key& key) const;

    iterator find(std::string_view key);

    const_iterator find(std::string_view key) const;

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

    void write_record(session& s);

    void read_record(session& s, const std::string& extra_condition = "");

    void read_record(const kind::sql_row& row);

    void delete_record(session& s);

    void create_table(session& s, bool if_not_exists=true);

    void drop_table(session& s, bool if_exists=true);

    void serialize(serializer& ser);

    void deserialize(deserializer& ser);

    model& operator>>(serializer& ser);

    model& operator>>(serializer&& ser);

    model& operator>>(session& s);

    model& operator<<(const kind::sql_row& row);

private:
    std::string table_;
    item_array items_;

    // Config
    bool update_duplicates_{true};
    bool ignore_insert_{false};
};

}// namespace dbm

#endif//DBM_MODEL_HPP



