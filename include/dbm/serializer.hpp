#ifndef DBM_SERIALIZER_HPP
#define DBM_SERIALIZER_HPP

#include <dbm/dbm_common.hpp>

namespace dbm {

class model;

struct serializer_base_tag
{
};

template<typename Derived>
struct serializer : public serializer_base_tag
{
    constexpr auto& self() { return static_cast<Derived&>(*this); }
    constexpr auto const& self() const { return static_cast<Derived const&>(*this); }

    template<typename = void>
    model& operator>>(model& m);

    template<typename = void>
    model&& operator>>(model&& m);
};

}// namespace dbm

#endif//DBM_SERIALIZER_HPP
