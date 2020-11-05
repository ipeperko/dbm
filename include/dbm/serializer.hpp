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

    virtual void serialize(std::string_view, int) = 0;

    virtual void serialize(std::string_view, short) = 0;

    virtual void serialize(std::string_view, long) = 0;

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

    virtual parse_result deserialize(std::string_view, int&) const = 0;

    virtual parse_result deserialize(std::string_view, short&) const = 0;

    virtual parse_result deserialize(std::string_view, long&) const = 0;

    virtual parse_result deserialize(std::string_view, double&) const = 0;

    virtual parse_result deserialize(std::string_view, std::string&) const = 0;

#ifdef DBM_EXPERIMENTAL_BLOB
    virtual parse_result deserialize(std::string_view, kind::blob&) const = 0;
#endif
};

}// namespace dbm

#endif//DBM_SERIALIZER_HPP
