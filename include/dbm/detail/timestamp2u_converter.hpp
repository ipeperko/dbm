#ifndef DBM_TIMESTAMP2U_CONVERTER_HPP
#define DBM_TIMESTAMP2U_CONVERTER_HPP

namespace dbm::kind::detail {

class timestamp2u_converter
{
public:
    using value_type = time_t;

    timestamp2u_converter() noexcept
        : local_(0)
        , ptr_(&local_)
        , is_ref_(false)
    {}

    // must be implicit
    timestamp2u_converter(value_type&& v) noexcept
        : local_(v)
        , ptr_(&local_)
        , is_ref_(false)
    {}

    // must be implicit
    timestamp2u_converter(value_type& v) noexcept
        : ptr_(&v)
        , is_ref_(true)
    {}

    timestamp2u_converter(const timestamp2u_converter& oth) noexcept
    {
        is_ref_ = oth.is_ref_;
        if (is_ref_) {
            ptr_ = oth.ptr_;
        }
        else {
            local_ = oth.local_;
            ptr_ = &local_;
        }
    }

    timestamp2u_converter(timestamp2u_converter&& oth) noexcept
    {
        is_ref_ = oth.is_ref_;
        if (is_ref_) {
            ptr_ = oth.ptr_;
            oth.ptr_ = &oth.local_; // bind other to local value to prevent crashes
            oth.is_ref_ = false;
        }
        else {
            local_ = oth.local_;
            ptr_ = &local_;
        }
    }

    ~timestamp2u_converter() = default;

    timestamp2u_converter& operator=(value_type v) noexcept
    {
        *ptr_ = v;
        return *this;
    }

    timestamp2u_converter& operator=(const timestamp2u_converter& oth) noexcept
    {
        if (this != &oth) {
            is_ref_ = oth.is_ref_;
            if (is_ref_) {
                ptr_ = oth.ptr_;
            }
            else {
                local_ = oth.local_;
                ptr_ = &local_;
            }
        }
        return *this;
    }

    timestamp2u_converter& operator=(timestamp2u_converter&& oth) noexcept
    {
        if (this != &oth) {
            is_ref_ = oth.is_ref_;
            if (is_ref_) {
                ptr_ = oth.ptr_;
                // bind other to his local value to prevent crashes
                oth.ptr_ = &oth.local_;
                oth.is_ref_ = false;
            }
            else {
                local_ = oth.local_;
                ptr_ = &local_;
            }
        }
        return *this;
    }

    // must be implicit
    operator value_type() noexcept
    {
        return *ptr_;
    }

    [[nodiscard]] value_type get() const noexcept
    {
        return *ptr_;
    }

    constexpr value_type* ptr() noexcept
    {
        return ptr_;
    }

    [[nodiscard]] bool has_reference() const noexcept
    {
        return is_ref_;
    }

private:
    value_type local_;  // local storage
    value_type* ptr_;   // pointer to external or local storage
    bool is_ref_;       // is reference flag
};

}
#endif //DBM_TIMESTAMP2U_CONVERTER_HPP
