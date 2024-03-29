#ifndef DBM_CONTAINER_HPP
#define DBM_CONTAINER_HPP

#include <dbm/dbm_common.hpp>
#include <dbm/serializer.hpp>
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

    void set_null(bool null) noexcept;

    constexpr bool is_null() const noexcept;

    void set_defined(bool defined) noexcept;

    constexpr bool is_defined() const noexcept;

    virtual kind::variant get() const = 0;

    virtual kind::variant get(size_t type_index) const = 0;

    template<typename T>
    T get() const;

    virtual void set(kind::variant& v) = 0;

    virtual void set(kind::variant&& v) = 0;

    virtual std::string to_string() const = 0;

    virtual void from_string(std::string_view v) = 0;

    virtual kind::data_type type() const noexcept = 0;

    virtual void* buffer() noexcept = 0;

    virtual size_t length() const noexcept = 0; // valid only for strings and blob

protected:
    bool is_null_ {true};
    bool defined_ {false};
};

DBM_INLINE container::container(bool null, bool defined)
    : is_null_(null)
    , defined_(defined)
{}

DBM_INLINE void container::set_null(bool null) noexcept
{
    is_null_ = null;
}

DBM_INLINE constexpr bool container::is_null() const noexcept
{
    return is_null_;
}

DBM_INLINE void container::set_defined(bool defined) noexcept
{
    defined_ = defined;
}

DBM_INLINE constexpr bool container::is_defined() const noexcept
{
    return defined_;
}

template<typename T>
T container::get() const
{
    /*
    kind::variant v = get();
    return std::get<T>(v);
     */
    auto v = get(kind::detail::variant_index<T>());
    return std::get<T>(v);
}

} // namespace dbm

#include <dbm/detail/container_impl.hpp>
#include <dbm/detail/impl/container_impl.ipp>

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

/*!
 * Make timestamp container
 *
 * @tparam Args
 * @param args
 * @return container unique pointer
 */
template <typename... Args>
auto timestamp(Args&&... args)
{
    return local<kind::detail::timestamp2u_converter>(std::forward<Args>(args)...);
}

/**
 * Runtime container factory
 *
 * @param type
 * @return container unique pointer
 */
inline dbm::container_ptr local_container_factory(dbm::kind::data_type type)
{
    switch (type) {
        case kind::data_type::Bool: return std::make_unique<detail::container_impl<bool>>();
        case kind::data_type::Int32: return std::make_unique<detail::container_impl<int32_t>>();
        case kind::data_type::Int16: return std::make_unique<detail::container_impl<int16_t>>();
        case kind::data_type::Int64: return std::make_unique<detail::container_impl<int64_t>>();
        case kind::data_type::UInt32: return std::make_unique<detail::container_impl<uint32_t>>();
        case kind::data_type::UInt16: return std::make_unique<detail::container_impl<uint16_t>>();
        case kind::data_type::UInt64: return std::make_unique<detail::container_impl<uint64_t>>();
        case kind::data_type::Timestamp2u: return std::make_unique<detail::container_impl<kind::detail::timestamp2u_converter>>();
        case kind::data_type::Double: return std::make_unique<detail::container_impl<double>>();
        case kind::data_type::String: return std::make_unique<detail::container_impl<std::string>>();
        default: return nullptr;
    }
}

}// namespace dbm

#endif//DBM_CONTAINER_HPP
