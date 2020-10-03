#ifndef DBM_NAMED_TYPE_HPP
#define DBM_NAMED_TYPE_HPP

#include <sstream>

namespace dbm::kind::detail {

// Curiously Recurring Template Pattern (CRTP)
template<typename T, template<typename> class CrtpType>
struct nt_crtp
{
    using object_type = T;

    constexpr auto& val() { return static_cast<object_type&>(*this); }

    constexpr auto const& val() const { return static_cast<object_type const&>(*this); }
};

// Printing named types
template<typename T>
struct printable : nt_crtp<T, printable>
{
    using object_type = typename nt_crtp<T, printable>::object_type;

    //
    // std::ostream printing
    //
    void print(std::ostream& os) const
    {
        os << this->val().get();
    }

    friend std::ostream& operator<<(std::ostream& os, const object_type& object)
    {
        object.print(os);
        return os;
    }

    //
    // Qt printing
    //
#ifdef QT_CORE_LIB
    void print(QDebug os) const
    {
        using value_type = typename object_type::value_type;

        if constexpr (std::is_same_v<std::string, value_type>) {
            os << this->val().get().c_str();
        }
        else {
            os << this->val().get();
        }
    }

    friend QDebug operator<<(QDebug os, const object_type& object)
    {
        object.print(os);
        return os;
    }
#endif // QT_CORE_LIB
};

// Hashing named type
template<typename T>
struct hashable : nt_crtp<T, hashable>
{
    static constexpr bool is_hashable = true;
};

// Named type
template<typename T, typename Parameter, template<typename> class... Skills>
class named_type : public Skills<named_type<T, Parameter, Skills...>>...
{
public:
    using value_type = T;

    // Disable default constructor
    named_type() = delete;

    // Construct from value type
    explicit named_type(value_type const& value)
        : value_(value)
    {}

    // Copy constructor
    named_type(named_type const& oth)
        : value_(oth.value_)
    {}

    // Construct by value type move
    explicit named_type(value_type&& value)
        : value_(std::move(value))
    {}

    // Move constructor
    named_type(named_type&& oth)
        : value_(std::move(oth.value_))
    {}

    // Destructor
    virtual ~named_type() = default;

    // Copy assignment
    named_type& operator=(const named_type& oth) = default;

    // Move assignment
    named_type& operator=(named_type&& oth) = default;

    // Comparison
    bool operator==(const named_type& oth) const
    {
        return value_ == oth.value_;
    }

    bool operator!=(const named_type& oth) const
    {
        return value_ != oth.value_;
    }

    // Value get
    constexpr auto& get() { return value_; }

    constexpr auto const& get() const { return value_; }

    // Empty string
    template<typename T1 = value_type, typename = std::enable_if_t<std::is_same_v<T1, std::string>>>
    bool empty() const
    {
        return value_.empty();
    }

private:
    value_type value_;
};

} // namespace dbm


// Hash for named types
namespace std {

template<typename T, typename Parameter, template<typename> class... Skills>
struct hash<dbm::kind::detail::named_type<T, Parameter, Skills...>>
{
    using named_type = dbm::kind::detail::named_type<T, Parameter, Skills...>;
    using checkIfHashable = typename enable_if<named_type::is_hashable, void>::type;

    size_t operator()(dbm::kind::detail::named_type<T, Parameter, Skills...> const& x) const
    {
        return hash<T>()(x.get());
    }
};

} // namespace std

#endif //DBM_NAMED_TYPE_HPP
