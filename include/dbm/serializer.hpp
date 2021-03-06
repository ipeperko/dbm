#ifndef DBM_SERIALIZER_HPP
#define DBM_SERIALIZER_HPP

#include <dbm/dbm_common.hpp>

namespace dbm {

class model;

class serializer
{
public:
    virtual ~serializer() = default;

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

#ifdef DBM_EXPERIMENTAL_BLOB
    virtual void serialize(std::string_view, const kind::blob&) = 0;
#endif
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

}// namespace dbm

#endif//DBM_SERIALIZER_HPP
