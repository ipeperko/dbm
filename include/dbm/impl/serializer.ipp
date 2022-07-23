#ifndef DBM_SERIALIZER_IPP
#define DBM_SERIALIZER_IPP

namespace dbm {
template<typename>
model& deserializer::operator>>(model& m)
{
    m.deserialize(*this);
    return m;
}

template<typename>
model&& deserializer::operator>>(model&& m)
{
    m.deserialize(*this);
    return std::move(m);
}

template<typename Derived>
template<typename>
model& serializer2<Derived>::operator>>(model& m)
{
    m.deserialize2(self());
    return m;
}

template<typename Derived>
template<typename>
model&& serializer2<Derived>::operator>>(model&& m)
{
    m.deserialize2(self());
    return std::move(m);
}

} // namespace dbm

#endif //DBM_SERIALIZER_IPP
