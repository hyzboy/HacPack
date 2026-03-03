#include "utf_conv.h"
#include <cstdint>
#include <functional>
#include <cstddef>

static inline uint32_t read_u32_le(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0]) ) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}
static inline uint32_t read_u32_be(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) | (static_cast<uint32_t>(p[2]) << 8) | (static_cast<uint32_t>(p[3]));
}

static void append_utf8_from_codepoint(std::string &out, uint32_t cp) {
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

static bool convert_utf32(const uint8_t* data, size_t size, std::function<uint32_t(const uint8_t*)> reader, std::string &out) {
    if (size % 4 != 0) return false;
    out.clear();
    for (size_t i = 0; i + 3 < size; i += 4) {
        uint32_t cp = reader(data + i);
        if (cp > 0x10FFFF) return false;
        if (cp >= 0xD800 && cp <= 0xDFFF) return false;
        append_utf8_from_codepoint(out, cp);
    }
    return true;
}

bool utf32le_bytes_to_utf8(const uint8_t* data, size_t size, std::string &out) {
    return convert_utf32(data, size, [](const uint8_t* p){ return read_u32_le(p); }, out);
}

bool utf32be_bytes_to_utf8(const uint8_t* data, size_t size, std::string &out) {
    return convert_utf32(data, size, [](const uint8_t* p){ return read_u32_be(p); }, out);
}
