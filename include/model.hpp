#ifndef DBM_MODEL_HPP
#define DBM_MODEL_HPP

#include "session.hpp"
#include "model_item.hpp"
#include <vector>

namespace dbm {

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

    explicit model(std::string table)
        : table_(std::move(table))
    {
    }

    model(std::string table, init_list l)
        : table_(std::move(table))
        , items_(l)
    {
    }

    model& operator=(const model&) = default;

    model& operator=(model&&) = default;

    virtual ~model() = default;

    const std::string& table_name() const
    {
        return table_;
    }

    void set_table_name(std::string name)
    {
        table_ = std::move(name);
    }

    void set_sql_duplicates_update(bool t)
    {
        update_duplicates_ = t;
    }

    void set_sql_ignore_insert(bool t)
    {
        ignore_insert_ = t;
    }

    model_item& at(const kind::key& key)
    {
        auto it = find(key);
        if (it == end()) {
            throw_exception<std::out_of_range>("Item not found '" + key.get() + "'");
        }
        return *it;
    }

    model_item const& at(const kind::key& key) const
    {
        auto it = find(key);
        if (it == end()) {
            throw_exception<std::out_of_range>("Item not found '" + key.get() + "'");
        }
        return *it;
    }

    model_item& at(std::string_view key)
    {
        auto it = find(key);
        if (it == end()) {
            throw_exception<std::out_of_range>("Item not found '" + std::string(key) + "'");
        }
        return *it;
    }

    model_item const& at(std::string_view key) const
    {
        auto it = find(key);
        if (it == end()) {
            throw_exception<std::out_of_range>("Item not found '" + std::string(key) + "'");
        }
        return *it;
    }

    iterator find(const kind::key& key)
    {
        return std::find_if(begin(), end(), [&](const model_item& item) {
            return item.key() == key;
        });
    }

    const_iterator find(const kind::key& key) const
    {
        return std::find_if(begin(), end(), [&](const model_item& item) {
            return item.key() == key;
        });
    }

    iterator find(std::string_view key)
    {
        return std::find_if(begin(), end(), [&](const model_item& item) {
            return item.key().get() == key;
        });
    }

    const_iterator find(std::string_view key) const
    {
        return std::find_if(begin(), end(), [&](const model_item& item) {
            return item.key().get() == key;
        });
    }

    auto& items() { return items_; }

    auto const& items() const { return items_; }

    iterator begin() { return items_.begin(); }

    const_iterator begin() const { return items_.begin(); }

    iterator end() { return items_.end(); }

    const_iterator end() const { return items_.end(); }

    template <typename ... Args>
    model_item& emplace_back(Args&&... args)
    {
        return items_.emplace_back(std::forward<Args>(args)...);
    }

    void erase(const kind::key& key)
    {
        auto it = find(key);
        if (it == end()) {
            throw_exception<std::out_of_range>("Cannot erase - item '" + key.get() + "' does not exist");
        }
        items_.erase(it);
    }

    void erase(std::string_view key)
    {
        auto it = find(key);
        if (it == end()) {
            throw_exception<std::out_of_range>("Cannot erase - item '" + std::string(key) + "' does not exist");
        }
        items_.erase(it);
    }

    void write_record(session& s);

    void read_record(session& s, const std::string& extra_condition = "");

    void read_record(const kind::sql_row& row);

    void delete_record(session& s);

    void create_table(session& s, bool if_not_exists=true)
    {
        s.create_table(*this, if_not_exists);
    }

    void drop_table(session& s, bool if_exists=true)
    {
        s.drop_table(*this, if_exists);
    }

    void serialize(serializer& ser)
    {
        for (auto& it : items_) {
            it.serialize(ser);
        }
    }

    void deserialize(deserializer& ser)
    {
        for (auto& it : items_) {
            it.deserialize(ser);
        }
    }

    model& operator>>(serializer& ser)
    {
        serialize(ser);
        return *this;
    }

    model& operator>>(serializer&& ser)
    {
        serialize(ser);
        return *this;
    }

    model& operator>>(session& s)
    {
        write_record(s);
        return *this;
    }

private:
    std::string table_;
    item_array items_;

    // Config
    bool update_duplicates_{true};
    bool ignore_insert_{false};
};

}// namespace dbm

#include "serializer.ipp"

#endif//DBM_MODEL_HPP



