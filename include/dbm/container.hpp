#ifndef DBM_CONTAINER_HPP
#define DBM_CONTAINER_HPP

#include <dbm/dbm_common.hpp>
#include <dbm/serializer.hpp>
#include <memory>

namespace dbm {

class container;
using container_ptr = std::unique_ptr<container>;

namespace detail {
class model_query_helper_base;
}

class container
{
public:
    enum config
    {
        no_narrow_cast = 0x1,
        no_int_to_double = 0x2
    };

    enum class init_null
    {
        defaults,
        not_null,
        null
    };

    enum class init_defined
    {
        defaults,
        not_defined,
        defined
    };


    container() = default;

    container(bool null, bool defined);

    virtual ~container() = default;

    virtual container_ptr clone() = 0;

    void set_null(bool null);

    constexpr bool is_null() const;

    void set_defined(bool defined);

    constexpr bool is_defined() const;

    virtual kind::variant get() const = 0;

    template<typename T>
    T get() const;

    virtual void set(kind::variant& v) = 0;

    virtual void set(kind::variant&& v) = 0;

    virtual std::string to_string() const = 0;

    virtual void from_string(std::string_view v) = 0;

    virtual void serialize(serializer& s, std::string_view tag) = 0;

    virtual bool deserialize(deserializer& s, std::string_view tag) = 0;

    virtual kind::data_type type() const = 0;

protected:
    bool is_null_ {true};
    bool defined_ {false};
};

DBM_INLINE container::container(bool null, bool defined)
    : is_null_(null)
    , defined_(defined)
{}

DBM_INLINE void container::set_null(bool null)
{
    is_null_ = null;
}

DBM_INLINE constexpr bool container::is_null() const
{
    return is_null_;
}

DBM_INLINE void container::set_defined(bool defined)
{
    defined_ = defined;
}

DBM_INLINE constexpr bool container::is_defined() const
{
    return defined_;
}

template<typename T>
T container::get() const
{
    kind::variant v = get();
    return std::get<T>(v);
}

} // namespace dbm

#include <dbm/detail/container_impl.hpp>
#include <dbm/detail/container_impl.ipp>

namespace dbm {

using init_null = container::init_null;
using init_defined = container::init_defined;

/**
 * Make container of type T with local storage
 *
 * After construction (init_null == defaults):
 * - null
 * - not defined
 *
 * If init_null is set to null or not_null:
 * - null depends on init_null
 * - defined
 *
 * @param validator function
 * @return container unique pointer
 */
template<typename T, detail::container_conf conf = 0>
container_ptr
local(std::function<bool(const T&)> validator = nullptr, init_null null = init_null::defaults)
{
    auto c = std::make_unique<detail::container_impl<T, detail::cont_value_local, conf>>(validator);

    if (null == init_null::null) {
        c->set_null(true);
        c->set_defined(true);
    }
    else if (null == init_null::not_null) {
        c->set_null(false);
        c->set_defined(true);
    }

    return c;
}

/**
 * Make container of type T with local storage
 *
 * After construction:
 * - not null
 * - defined
 *
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
        val, validator);
}

/**
 * Make container of type T with local storage
 *
 * After construction:
 * - not null
 * - defined
 *
 * @param val value reference
 * @param validator function
 * @return container unique pointer
 */
template<typename T,
         detail::container_conf conf = 0,
         typename = std::enable_if_t<not std::is_lvalue_reference_v<T>, void>>
container_ptr
local(T&& val, std::function<bool(const T&)> validator = nullptr)
{
    if (validator && !validator(val)) {
        throw_exception<std::out_of_range>("container validator failed");
    }

    return std::make_unique<detail::container_impl<T, detail::cont_value_local, conf>>(
        std::forward<T>(val), validator);
}

/**
 * Make container of type T with binding
 *
 * @param value reference of type T
 * @param validator object or nullptr
 * @param value null parameter
 * @param value defined parameter
 * @return container unique pointer
 */
template<typename T, detail::container_conf conf = 0, class Validator = std::nullptr_t>
container_ptr binding(T& ref, Validator&& validator = nullptr,
                      init_null null = init_null::not_null,
                      init_defined defined = init_defined::defined)
{
    if (null == init_null::defaults) {
        null = init_null::not_null;
    }

    if (defined == init_defined::defaults) {
        defined = init_defined::defined;
    }

    return std::make_unique<detail::container_impl<T, detail::cont_value_binding, conf>>(
        detail::cont_value_binding<T>(ref),
        validator,
        null == init_null::null,
        defined == init_defined::defined);
}

#define timestamp local<dbm::timestamp2u_converter>

}// namespace dbm

#endif//DBM_CONTAINER_HPP
