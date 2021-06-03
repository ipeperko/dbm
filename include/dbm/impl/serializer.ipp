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

}

#endif //DBM_SERIALIZER_IPP
