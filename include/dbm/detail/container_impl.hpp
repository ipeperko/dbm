#ifndef DBM_CONTAINER_IMPL_HPP
#define DBM_CONTAINER_IMPL_HPP

namespace dbm::detail {

using container_conf = uint32_t;

template<typename T>
class cont_value_local
{
public:
    using value_type = T;
    using unreferenced_type = T;
};

template<typename T>
class cont_value_binding
{
public:
    using value_type = T&;
    using unreferenced_type = T;

    cont_value_binding(value_type v)
        : val_(v)
    {}

    value_type val_;
};

template<typename T, template<typename> class ContType = cont_value_local, container_conf conf = 0>
class container_impl : public container
{
public:
    using value_type = typename ContType<T>::value_type;
    using unreferenced_type = typename ContType<T>::unreferenced_type;
    using reference_type = unreferenced_type&;
    using validator_fn = std::function<bool(const unreferenced_type&)>;
    static constexpr bool is_reference_storage = std::is_same_v<ContType<T>, cont_value_binding<T>>;

    static_assert(kind::detail::variant_index<unreferenced_type>() != std::variant_npos,
                  "Container invalid type");
    static_assert(not std::is_same_v<unreferenced_type, const char*> && not std::is_same_v<unreferenced_type, std::string_view>,
                  "Container invalid string type");
    static_assert(not (std::is_same_v<unreferenced_type, kind::detail::timestamp2u_converter> && is_reference_storage),
                  "timestamp2u_converter cannot be in reference container type");

    container_impl()
        : container()
        , val_ {}
    {
    }

    explicit container_impl(validator_fn validator)
        : container()
        , val_ {}
        , validator_(std::move(validator))
    {
    }

    template <typename Dummy = void, typename = std::enable_if_t<not is_reference_storage, Dummy>>
    container_impl(T&& val, validator_fn validator)
        : container(false, true)
        , val_ (std::move(val))
        , validator_(std::move(validator))
    {
    }


    template <typename Dummy = void, typename = std::enable_if_t<not is_reference_storage, Dummy>>
    container_impl(const T& val, validator_fn validator)
        : container(false, true)
        , val_ (val)
        , validator_(std::move(validator))
    {
    }

    container_impl(cont_value_binding<T>&& v, validator_fn validator, bool null, bool defined)
        : container(null, defined)
        , val_(v.val_)
        , validator_(std::move(validator))
    {
    }

    container_ptr clone() override
    {
        if constexpr (is_reference_storage) {
            return std::make_unique<container_impl<T, ContType, conf>>(ContType<T>(val_), validator_, is_null_, defined_);
        }
        else {
            return std::make_unique<container_impl<T, ContType, conf>>(val_, validator_, is_null_, defined_);
        }
    }

    std::string to_string() const override;

    void from_string(std::string_view v) override;

    kind::variant get() const override
    {
        return { val_ };
    }

    kind::variant get(size_t type_index) const override;

    void set(kind::variant& v) override;

    void set(kind::variant&& v) override
    {
        set(v);
    }

    void serialize(serializer& s, std::string_view tag) override;

    bool deserialize(deserializer& s, std::string_view tag) override;

    kind::data_type type() const noexcept override
    {
        return static_cast<kind::data_type>(kind::detail::variant_index<unreferenced_type>());
    }

    void* buffer() noexcept override { return &val_; }

    size_t length() const noexcept override;

private:
    value_type val_;
    validator_fn validator_;
};

} // namespace

#endif //DBM_CONTAINER_IMPL_HPP
