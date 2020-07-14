#ifndef DBM_NAMED_TYPE_HPP
#define DBM_NAMED_TYPE_HPP

#include <sstream>

namespace dbm::kind::detail {

template<typename T, template<typename> class CrtpType>
struct nt_crtp
{
    T& val() { return static_cast<T&>(*this); }

    T const& val() const { return static_cast<T const&>(*this); }
};

template<typename T>
struct printable : nt_crtp<T, printable>
{
    void print(std::ostream& os) const
    {
        os << this->val().get();
    }

    friend std::ostream& operator<<(std::ostream& os, const T& object)
    {
        object.print(os);
        return os;
    }
};

template<typename T>
struct hashable : nt_crtp<T, hashable>
{
    static constexpr bool is_hashable = true;
};

template<typename T, typename Parameter, template<typename> class... Skills>
class named_type : public Skills<named_type<T, Parameter, Skills...>>...
{
public:
    explicit named_type(T const& value)
        : value_(value)
    {}

    named_type(named_type const& oth)
        : value_(oth.value_)
    {}

    explicit named_type(T&& value)
        : value_(std::move(value))
    {}

    named_type(named_type&& oth)
        : value_(std::move(oth.value_))
    {}

    named_type& operator=(const named_type& oth) = default;

    named_type& operator=(named_type&& oth) = default;

    bool operator==(const named_type& oth) const
    {
        return value_ == oth.value_;
    }

    bool operator!=(const named_type& oth) const
    {
        return value_ != oth.value_;
    }

    T& get() { return value_; }

    T const& get() const { return value_; }

    template<typename T1 = T, typename = std::enable_if_t<std::is_same_v<T1, std::string>>>
    bool empty() const
    {
        return value_.empty();
    }

private:
    T value_;
};

} // namespace dbm

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
