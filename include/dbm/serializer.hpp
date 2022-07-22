#ifndef DBM_SERIALIZER_HPP
#define DBM_SERIALIZER_HPP

#include <dbm/dbm_common.hpp>

#define DBM_USING_SERIALIZE using dbm::serializer::serialize
#define DBM_USING_DESERIALIZE using dbm::deserializer::deserialize

#define DBM_SERIALIZE_GENERIC_FUNC(GenericFunc)  \
    GenericFunc(std::nullptr_t)             \
    GenericFunc(bool)                       \
    GenericFunc(int32_t)                    \
    GenericFunc(int16_t)                    \
    GenericFunc(int64_t)                    \
    GenericFunc(uint32_t)                   \
    GenericFunc(uint16_t)                   \
    GenericFunc(uint64_t)                   \
    GenericFunc(double)                     \
    GenericFunc(std::string const&)

#define DBM_SERIALIZE_GENERIC_FUNC_ALL_STRING(GenericFunc) \
    DBM_SERIALIZE_GENERIC_FUNC(GenericFunc)                \
    GenericFunc(const char*)                               \
    GenericFunc(std::string_view)

#define DBM_DESERIALIZE_GENERIC_FUNC(GenericFunc) \
    GenericFunc(bool)                       \
    GenericFunc(int32_t)                    \
    GenericFunc(int16_t)                    \
    GenericFunc(int64_t)                    \
    GenericFunc(uint32_t)                   \
    GenericFunc(uint16_t)                   \
    GenericFunc(uint64_t)                   \
    GenericFunc(double)                     \
    GenericFunc(std::string)


namespace dbm {

class model;

class serializer
{
public:
    virtual ~serializer() = default;

    dbm::model& operator>>(dbm::model& m)
    {
        deserialize(m);
        return m;
    }

    dbm::model&& operator>>(dbm::model&& m)
    {
        deserialize(m);
        return std::move(m);
    }

    /*
     * All methods should be pure virtual in order to force
     * implementation to handle all types
     */

    virtual void serialize(std::string_view, std::nullptr_t) = 0;

    virtual void serialize(std::string_view, bool) = 0;

    virtual void serialize(std::string_view, int32_t) = 0;

    virtual void serialize(std::string_view, int16_t) = 0;

    virtual void serialize(std::string_view, int64_t) = 0;

    virtual void serialize(std::string_view, uint32_t) = 0;

    virtual void serialize(std::string_view, uint16_t) = 0;

    virtual void serialize(std::string_view, uint64_t) = 0;

    virtual void serialize(std::string_view, double) = 0;

    virtual void serialize(std::string_view, const std::string&) = 0;

    virtual void serialize(std::string_view key, const char* val)
    {
        serialize(key, std::string(val));
    }

    virtual void serialize(std::string_view key, std::string_view val)
    {
        serialize(key, std::string(val));
    }

#ifdef DBM_EXPERIMENTAL_BLOB
    virtual void serialize(std::string_view, const kind::blob&) = 0;
#endif

protected:
    virtual void deserialize(dbm::model&)
    {}
};

class deserializer
{
public:
    enum parse_result
    {
        error,
        undefined,
        ok,
        null
    };

    virtual ~deserializer() = default;

    template<typename = void>
    model& operator>>(model& m);

    template<typename = void>
    model&& operator>>(model&& m);

    /*
     * All methods should be pure virtual in order to force
     * implementation to handle all types
     */

    virtual parse_result deserialize(std::string_view, bool&) const = 0;

    virtual parse_result deserialize(std::string_view, int32_t&) const = 0;

    virtual parse_result deserialize(std::string_view, int16_t&) const = 0;

    virtual parse_result deserialize(std::string_view, int64_t&) const = 0;

    virtual parse_result deserialize(std::string_view, uint32_t&) const = 0;

    virtual parse_result deserialize(std::string_view, uint16_t&) const = 0;

    virtual parse_result deserialize(std::string_view, uint64_t&) const = 0;

    virtual parse_result deserialize(std::string_view, double&) const = 0;

    virtual parse_result deserialize(std::string_view, std::string&) const = 0;

#ifdef DBM_EXPERIMENTAL_BLOB
    virtual parse_result deserialize(std::string_view, kind::blob&) const = 0;
#endif
};

struct serializer_base_tag
{};

template<typename Derived>
struct serializer2 : public serializer_base_tag
{
    auto& self() { return static_cast<Derived&>(*this); }
    auto const& self() const { return static_cast<Derived const&>(*this); }
};

struct deserializer_base_tag
{
    enum parse_result
    {
        error,
        undefined,
        ok,
        null
    };
};

}// namespace dbm

#endif//DBM_SERIALIZER_HPP
