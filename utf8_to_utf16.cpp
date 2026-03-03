#include "utf_conv.h"

#include <string>
#include <cstdint>

bool utf8_to_utf16(const std::string &utf8, std::u16string &out)
{
    out.clear();
    if (utf8.empty()) return true;

    const size_t len = utf8.size();
    out.reserve(len);

    size_t i = 0;
    while (i < len) {
        uint8_t b0 = static_cast<uint8_t>(utf8[i]);
        uint32_t codepoint = 0;
        size_t extra = 0;

        if (b0 <= 0x7F) {
            codepoint = b0;
            extra = 0;
        } else if ((b0 & 0xE0) == 0xC0) {
            // 2-byte sequence
            codepoint = b0 & 0x1F;
            extra = 1;
            if (b0 < 0xC2) return false; // overlong
        } else if ((b0 & 0xF0) == 0xE0) {
            // 3-byte sequence
            codepoint = b0 & 0x0F;
            extra = 2;
        } else if ((b0 & 0xF8) == 0xF0) {
            // 4-byte sequence
            codepoint = b0 & 0x07;
            extra = 3;
            if (b0 > 0xF4) return false; // > U+10FFFF
        } else {
            return false; // invalid leading byte
        }

        if (i + extra >= len) return false; // truncated

        for (size_t k = 1; k <= extra; ++k) {
            uint8_t bx = static_cast<uint8_t>(utf8[i + k]);
            if ((bx & 0xC0) != 0x80) return false; // invalid continuation
            codepoint = (codepoint << 6) | (bx & 0x3F);
        }

        // Check for overlong encodings
        if (extra == 1) {
            if (codepoint < 0x80) return false;
        } else if (extra == 2) {
            if (codepoint < 0x800) return false;
        } else if (extra == 3) {
            if (codepoint < 0x10000) return false;
        }

        // Reject surrogates and invalid codepoints
        if (codepoint >= 0xD800 && codepoint <= 0xDFFF) return false;
        if (codepoint > 0x10FFFF) return false;

        // Encode to UTF-16
        if (codepoint <= 0xFFFF) {
            out.push_back(static_cast<char16_t>(codepoint));
        } else {
            uint32_t cp = codepoint - 0x10000;
            char16_t hi = static_cast<char16_t>(0xD800 + ((cp >> 10) & 0x3FF));
            char16_t lo = static_cast<char16_t>(0xDC00 + (cp & 0x3FF));
            out.push_back(hi);
            out.push_back(lo);
        }

        i += 1 + extra;
    }

    return true;
}
