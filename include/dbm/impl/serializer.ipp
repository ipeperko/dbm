#ifndef DBM_SERIALIZER_IPP
#define DBM_SERIALIZER_IPP

namespace dbm {

template<typename Derived>
template<typename>
model& serializer<Derived>::operator>>(model& m)
{
    m.deserialize(self());
    return m;
}

template<typename Derived>
template<typename>
model&& serializer<Derived>::operator>>(model&& m)
{
    m.deserialize(self());
    return std::move(m);
}

} // namespace dbm

#endif //DBM_SERIALIZER_IPP
