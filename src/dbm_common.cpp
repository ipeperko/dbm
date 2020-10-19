#include "dbm_common.hpp"

namespace dbm {

// ----------------------------------------------------------
// Exception
// ----------------------------------------------------------

//namespace config {
//static std::function<void(std::string const&)> custom_except_fn;
//
//void set_custom_exception(std::function<void(std::string const&)> except_fn)
//{
//    custom_except_fn = std::move(except_fn);
//}
//
//std::function<void(std::string const&)>& get_custom_exception()
//{
//    return custom_except_fn;
//}
//} // namespace config

// ----------------------------------------------------------
// base 64
// ----------------------------------------------------------

//namespace utils {
//
//static const std::string_view base64_chars =
//    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
//    "abcdefghijklmnopqrstuvwxyz"
//    "0123456789+/";
//
//static inline bool is_base64(unsigned char c)
//{
//    return (isalnum(c) || (c == '+') || (c == '/'));
//}
//
//static inline bool char_skip(char c)
//{
//    return (c == '\n');
//}
//
//std::string b64_encode(const void* _bytes_to_encode, size_t in_len)
//{
//    std::string encoded;
//
//    auto* data = reinterpret_cast<unsigned char const*>(_bytes_to_encode);
//    int i = 0;
//    int j = 0;
//    unsigned char char_array_3[3];
//    unsigned char char_array_4[4];
//
//    while (in_len--) {
//        char_array_3[i++] = *(data++);
//        if (i == 3) {
//            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
//            char_array_4[1] = ((char_array_3[0] & 0x03) << 4)
//                + ((char_array_3[1] & 0xf0) >> 4);
//            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2)
//                + ((char_array_3[2] & 0xc0) >> 6);
//            char_array_4[3] = char_array_3[2] & 0x3f;
//
//            for (i = 0; (i < 4); i++)
//                encoded += base64_chars[char_array_4[i]];
//            i = 0;
//        }
//    }
//
//    if (i) {
//        for (j = i; j < 3; j++)
//            char_array_3[j] = '\0';
//
//        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
//        char_array_4[1] = ((char_array_3[0] & 0x03) << 4)
//            + ((char_array_3[1] & 0xf0) >> 4);
//        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2)
//            + ((char_array_3[2] & 0xc0) >> 6);
//        char_array_4[3] = char_array_3[2] & 0x3f;
//
//        for (j = 0; (j < i + 1); j++)
//            encoded += base64_chars[char_array_4[j]];
//
//        while ((i++ < 3))
//            encoded += '=';
//    }
//
//    return encoded;
//}
//
//std::string b64_decode(std::string_view s)
//{
//    std::string decoded;
//    int in_len = s.size();
//    int i = 0;
//    int j = 0;
//    int in_ = 0;
//    unsigned char char_array_4[4], char_array_3[3];
//
//    while (in_len-- && (s[in_] != '=')) {
//
//        if (char_skip(s[in_])) {
//            in_++;
//            continue;
//        }
//
//        if (!is_base64(s[in_])) {
//            break;
//        }
//
//        char_array_4[i++] = s[in_];
//        in_++;
//        if (i == 4) {
//            for (i = 0; i < 4; i++)
//                char_array_4[i] = base64_chars.find(char_array_4[i]);
//
//            char_array_3[0] = (char_array_4[0] << 2)
//                + ((char_array_4[1] & 0x30) >> 4);
//            char_array_3[1] = ((char_array_4[1] & 0xf) << 4)
//                + ((char_array_4[2] & 0x3c) >> 2);
//            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
//
//            for (i = 0; (i < 3); i++)
//                decoded += char_array_3[i];
//            i = 0;
//        }
//    }
//
//    if (i) {
//        for (j = i; j < 4; j++)
//            char_array_4[j] = 0;
//
//        for (j = 0; j < 4; j++)
//            char_array_4[j] = base64_chars.find(char_array_4[j]);
//
//        char_array_3[0] = (char_array_4[0] << 2)
//            + ((char_array_4[1] & 0x30) >> 4);
//        char_array_3[1] = ((char_array_4[1] & 0xf) << 4)
//            + ((char_array_4[2] & 0x3c) >> 2);
//        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
//
//        for (j = 0; (j < i - 1); j++)
//            decoded += char_array_3[j];
//    }
//
//    return decoded;
//}

//} // namespace utils
} // namespace dbm
