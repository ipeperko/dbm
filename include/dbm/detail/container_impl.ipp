#ifndef DBM_CONTAINER_IMPL_IPP
#define DBM_CONTAINER_IMPL_IPP

#ifdef DBM_EXPERIMENTAL_CHARCONV
#include <charconv>
#endif

namespace dbm::detail {

template<typename T, template<typename> class ContType, container_conf conf>
std::string container_impl<T, ContType, conf>::to_string() const
{
    if (is_null_) {
        throw_exception<std::runtime_error>("Container value is null - cannot convert to string");
    }

    if constexpr (std::is_same_v<unreferenced_type, std::string>) {
        return val_;
    }
    else if constexpr (std::is_same_v<unreferenced_type, kind::detail::timestamp2u_converter>) {
        return std::to_string(val_.get());
    }
    else {
        std::ostringstream os;
        os << val_;
        return os.str();
    }
}

template<typename T, template<typename> class ContType, container_conf conf>
void container_impl<T, ContType, conf>::from_string(std::string_view v)
{
    auto on_error_exception = [this](const char* msg) {
        is_null_ = true;
        defined_ = false;
        throw_exception<std::runtime_error>(msg);
    };

    auto validate = [&](const unreferenced_type& val) {
        if (validator_ && !validator_(val)) {
            on_error_exception("From string validator failed");
        }
    };

    if constexpr (std::is_same_v<unreferenced_type, std::string>) {
        std::string tmp_val {v};
        validate(tmp_val);
        val_ = std::move(tmp_val);
        is_null_ = false;
        defined_ = true;
    }
    else if constexpr (std::is_same_v<unreferenced_type, kind::detail::timestamp2u_converter>) {
        kind::detail::timestamp2u_converter::value_type tmp_val;
        utils::istream_extbuf is(const_cast<char*>(&v[0]), v.length());
        is >> tmp_val;
        if (is.fail()) {
            on_error_exception("Container from_string failed");
        }
        validate(tmp_val);
        val_ = tmp_val;
        is_null_ = false;
        defined_ = true;
    }
#ifdef DBM_EXPERIMENTAL_BLOB
    else if constexpr (std::is_same_v<T, kind::blob>) {
        std::copy(v.begin(), v.end(), std::back_inserter(tmp_val));
        validate(tmp_val);
        val_ = std::move(tmp_val);
        is_null_ = false;
        defined_ = true;
    }
#endif
#ifdef DBM_EXPERIMENTAL_CHARCONV
    else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, long> || std::is_same_v<T, short>) {
        unreferenced_type tmp_val;
        auto res = std::from_chars(v.data(), v.data() + v.length(), tmp_val);

        if (res.ec != std::errc()) {
            on_error_exception("Container from_string failed");
        }

        validate(tmp_val);
        val_ = std::move(tmp_val);
        is_null_ = false;
        defined_ = true;
    }
//    else if constexpr (std::is_same_v<T, double>) {
//        double d;
//        auto res = std::from_chars(v.data(), v.data() + v.length(), d);
//    }
#endif
    else {
        unreferenced_type tmp_val;
        utils::istream_extbuf is(const_cast<char*>(&v[0]), v.length());
        is >> tmp_val;
        if (is.fail()) {
            on_error_exception("Container from_string failed");
        }
        validate(tmp_val);
        val_ = std::move(tmp_val);
        is_null_ = false;
        defined_ = true;
    }
}

template<typename T, template<typename> class ContType, container_conf conf>
void container_impl<T, ContType, conf>::set(kind::variant& v)
{
    auto on_error = [this](const char* msg) {
        is_null_ = true;
        defined_ = false;
        throw_exception<std::domain_error>(msg);
    };

    if (std::holds_alternative<std::nullptr_t>(v)) {
        is_null_ = true;
        defined_ = true;
        return;
    }

    unreferenced_type tmp_val;

    if constexpr (std::is_integral_v<unreferenced_type>) {

        if (std::holds_alternative<unreferenced_type>(v)) {
            tmp_val = std::get<unreferenced_type>(v);
        }
        else if (conf & no_narrow_cast) {
            on_error("Setting container integral value from incompatible type");
        }
        else if (std::holds_alternative<double>(v)) {
            tmp_val = std::get<double>(v);
            // TODO: lost precision warning maybe?
        }
        else {
            if (utils::arithmetic_convert<int64_t>(tmp_val, v)) {
            }
            else if (utils::arithmetic_convert<int32_t>(tmp_val, v)) {
            }
            else if (utils::arithmetic_convert<int16_t>(tmp_val, v)) {
            }
            else if (utils::arithmetic_convert<uint64_t>(tmp_val, v)) {
            }
            else if (utils::arithmetic_convert<uint32_t>(tmp_val, v)) {
            }
            else if (utils::arithmetic_convert<uint16_t>(tmp_val, v)) {
            }
            else if (utils::arithmetic_convert<bool>(tmp_val, v)) {
            }
            else {
                on_error("Setting container integral value from unsupported value type");
            }
        }
    }
    else if constexpr (std::is_floating_point_v<unreferenced_type>) {

        if (std::holds_alternative<unreferenced_type>(v)) {
            tmp_val = std::get<unreferenced_type>(v);
        }
        else if (conf & no_int_to_double) {
            on_error("Setting container floating type value from incompatible type");
        }
        else {
            if (std::holds_alternative<int64_t>(v)) {
                tmp_val = std::get<int64_t>(v);
            }
            else if (std::holds_alternative<int32_t>(v)) {
                tmp_val = std::get<int32_t>(v);
            }
            else if (std::holds_alternative<int16_t>(v)) {
                tmp_val = std::get<int16_t>(v);
            }
            else if (std::holds_alternative<uint64_t>(v)) {
                tmp_val = std::get<uint64_t>(v);
            }
            else if (std::holds_alternative<uint32_t>(v)) {
                tmp_val = std::get<uint32_t>(v);
            }
            else if (std::holds_alternative<uint16_t>(v)) {
                tmp_val = std::get<uint16_t>(v);
            }
            else if (std::holds_alternative<bool>(v)) {
                tmp_val = std::get<bool>(v);
            }
            else {
                on_error("Setting container floating type value from incompatible type");
            }
        }
    }
    else if constexpr (std::is_same_v<std::string, unreferenced_type>) {

        if (std::holds_alternative<std::string>(v)) {
            tmp_val = std::move(std::get<std::string>(v));
        }
        else if (std::holds_alternative<const char*>(v)) {
            tmp_val = std::get<const char*>(v);
        }
        else if (std::holds_alternative<std::string_view>(v)) {
            tmp_val = std::get<std::string_view>(v);
        }
        else {
            on_error("Unsupported string type");
        }
    }
    else {
        tmp_val = std::move(std::get<unreferenced_type>(v));
    }

    is_null_ = false;

    if (validator_) {
        if (!validator_(tmp_val)) {
            on_error("Validator failed");
        }
    }

    val_ = std::move(tmp_val);
    defined_ = true;
}

template<typename T, template<typename> class ContType, container_conf conf>
void container_impl<T, ContType, conf>::serialize(serializer& s, std::string_view tag)
{
    if (is_null()) {
        s.serialize(tag, nullptr);
    }
    else {
        if constexpr (std::is_same_v<unreferenced_type, kind::detail::timestamp2u_converter>) {
            if constexpr (std::is_same_v<time_t, int64_t>) {
                s.serialize(tag, val_.get());
            }
            else {
                s.serialize(tag, static_cast<int64_t>(val_.get()));
            }
        }
        else {
            s.serialize(tag, val_);
        }
    }
}

template<typename T, template<typename> class ContType, container_conf conf>
bool container_impl<T, ContType, conf>::deserialize(deserializer& s, std::string_view tag)
{
    unreferenced_type tmp_val;
    deserializer::parse_result res;

    if constexpr (std::is_same_v<unreferenced_type, kind::detail::timestamp2u_converter>) {
        // will be deserialized as long int
        if constexpr (std::is_same_v<time_t, int64_t>) {
            res = s.deserialize(tag, *tmp_val.ptr()); 
        }
        else {
            int64_t val64;
            res = s.deserialize(tag, val64); 
            tmp_val = static_cast<kind::detail::timestamp2u_converter::value_type>(val64);
        }
    }
    else {
        res = s.deserialize(tag, tmp_val);
    }

    switch (res) {
        case deserializer::ok:
            if (validator_ && !validator_(tmp_val)) {
                is_null_ = true;
                defined_ = false;
                throw_exception<std::domain_error>("Deserialize failed validate - tag " + std::string(tag));
            }
            val_ = std::move(tmp_val);
            is_null_ = false;
            defined_ = true;
            return true;
        case deserializer::null:
            is_null_ = true;
            defined_ = true;
            return true;
        case deserializer::undefined:
            // do not change anything
            return false;
        case deserializer::error:
            [[fallthrough]];
        default:
            is_null_ = true;
            defined_ = false;
            throw_exception<std::domain_error>("Deserialize failed - tag " + std::string(tag));
    }
}

} // namespace dbm

#endif
