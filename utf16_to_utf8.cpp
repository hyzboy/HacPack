#include "utf_conv.h"

#include <vector>
#include <functional>

static inline uint16_t read_u16_le(const uint8_t* p) { return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8); }
static inline uint16_t read_u16_be(const uint8_t* p) { return static_cast<uint16_t>(p[0]) << 8 | static_cast<uint16_t>(p[1]); }

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

static bool convert_utf16(const uint8_t* data, size_t size, std::function<uint16_t(const uint8_t*)> reader, std::string &out) {
    if (size % 2 != 0) return false;
    out.clear();
    size_t i = 0;
    while (i + 1 < size) {
        uint16_t w1 = reader(data + i);
        i += 2;
        if (w1 >= 0xD800 && w1 <= 0xDBFF) {
            if (i + 1 >= size) return false;
            uint16_t w2 = reader(data + i);
            i += 2;
            if (w2 < 0xDC00 || w2 > 0xDFFF) return false;
            uint32_t cp = 0x10000 + (((w1 - 0xD800) << 10) | (w2 - 0xDC00));
            append_utf8_from_codepoint(out, cp);
        } else if (w1 >= 0xDC00 && w1 <= 0xDFFF) {
            return false;
        } else {
            append_utf8_from_codepoint(out, w1);
        }
    }
    return true;
}

bool utf16le_bytes_to_utf8(const uint8_t* data, size_t size, std::string &out) {
    return convert_utf16(data, size, [](const uint8_t* p){ return read_u16_le(p); }, out);
}

bool utf16be_bytes_to_utf8(const uint8_t* data, size_t size, std::string &out) {
    return convert_utf16(data, size, [](const uint8_t* p){ return read_u16_be(p); }, out);
}

bool utf16_string_to_utf8(const std::u16string &in, std::string &out) {
    out.clear();
    out.reserve(in.size());
    size_t i = 0;
    while (i < in.size()) {
        char16_t w1 = in[i++];
        uint32_t cp = 0;
        if (w1 >= 0xD800 && w1 <= 0xDBFF) {
            if (i >= in.size()) return false;
            char16_t w2 = in[i++];
            if (w2 < 0xDC00 || w2 > 0xDFFF) return false;
            cp = 0x10000 + (((static_cast<uint32_t>(w1) - 0xD800) << 10) | (static_cast<uint32_t>(w2) - 0xDC00));
        } else if (w1 >= 0xDC00 && w1 <= 0xDFFF) {
            return false;
        } else {
            cp = static_cast<uint32_t>(w1);
        }
        append_utf8_from_codepoint(out, cp);
    }
    return true;
}
