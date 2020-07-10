#ifndef DBM_VALUE_HPP
#define DBM_VALUE_HPP

#include "dbm_common.hpp"
#include "serializer.hpp"
#include <memory>

namespace dbm {

class container;
using container_ptr = std::unique_ptr<container>;

class container
{
public:
    enum config
    {
        no_narrow_cast = 0x1,
        no_int_to_double = 0x2
    };

    container() = default;

    container(bool null, bool defined)
        : is_null_(null)
        , defined_(defined)
    {}

    virtual ~container() = default;

    virtual container_ptr clone() = 0;

    void set_null(bool null)
    {
        is_null_ = null;
    }

    bool is_null() const
    {
        return is_null_;
    }

    void set_defined(bool defined)
    {
        defined_ = defined;
    }

    bool is_defined() const
    {
        return defined_;
    }

    virtual kind::variant get() const = 0;

    template<typename T>
    T get() const
    {
        kind::variant v = get();
        return std::get<T>(v);
    }

    virtual void set(kind::variant& v) = 0;
    virtual void set(kind::variant&& v) = 0;
    virtual std::string to_string() const = 0;
    virtual void from_string(std::string_view v) = 0;
    virtual void serialize(serializer& s, std::string_view tag) = 0;
    virtual bool deserialize(deserializer& s, std::string_view tag) = 0;

protected:
    bool is_null_ {true};
    bool defined_ {false};
};

} // namespace dbm

#include "detail/container_impl.hpp"
#include "detail/container_impl.ipp"

namespace dbm {

/**
 * Make container of type T with local storage
 * @param validator function
 * @return container unique pointer
 */
template<typename T, detail::container_conf conf = 0>
container_ptr
local(std::function<bool(const T&)> validator = nullptr)
{
    return std::make_unique<detail::container_impl<T, detail::cont_value_local, conf>>(validator);
}

/**
 * Make container of type T with local storage
 * @param val value reference
 * @param validator function
 * @return container unique pointer
 */
template<typename T, detail::container_conf conf = 0>
container_ptr
local(const T& val, std::function<bool(const T&)> validator = nullptr)
{
    if (validator && !validator(val)) {
        throw_exception<std::out_of_range>("container validator failed");
    }

    return std::make_unique<detail::container_impl<T, detail::cont_value_local, conf>>(
        val,
        validator);
}

/**
 * Make container of type T with local storage
 * @param val value reference
 * @param validator function
 * @return container unique pointer
 */
template<typename T, detail::container_conf conf = 0, typename = std::enable_if_t<not std::is_lvalue_reference_v<T>, void>>
container_ptr
local(T&& val, std::function<bool(const T&)> validator = nullptr)
{
    if (validator && !validator(val)) {
        throw_exception<std::out_of_range>("container validator failed");
    }

    return std::make_unique<detail::container_impl<T, detail::cont_value_local, conf>>(
        std::forward<T>(val),
        validator);
}

/**
 * Make container of type T with binding
 * @param value reference of type T
 * @param validator object or nullptr
 * @param value null parameter
 * @param value defined parameter
 * @return container unique pointer
 */
template<typename T, detail::container_conf conf = 0, class Validator = std::nullptr_t>
container_ptr binding(T& ref, Validator&& validator = nullptr, bool null = false, bool defined = true)
{
    return std::make_unique<detail::container_impl<T, detail::cont_value_binding, conf>>(
            detail::cont_value_binding<T>(ref), validator, null, defined);
}

}// namespace dbm

#endif//DBM_VALUE_HPP
