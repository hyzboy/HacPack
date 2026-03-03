#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

// Convert UTF-8 string to UTF-16 (std::u16string).
// Returns true on success, false on failure.
bool utf8_to_utf16(const std::string &utf8, std::u16string &out);

// Convert raw UTF-16 bytes (without BOM) to UTF-8 string.
// `data` points to the bytes, `size` is the byte count (should be even).
// Returns true on success, false on invalid input (e.g., odd size or invalid surrogates).
bool utf16le_bytes_to_utf8(const uint8_t* data, size_t size, std::string &out);
bool utf16be_bytes_to_utf8(const uint8_t* data, size_t size, std::string &out);

// Convert raw UTF-32 bytes (without BOM) to UTF-8 string.
// `data` points to the bytes, `size` is the byte count (should be multiple of 4).
// Returns true on success, false on invalid input (e.g., size not multiple of 4 or invalid codepoints).
bool utf32le_bytes_to_utf8(const uint8_t* data, size_t size, std::string &out);
bool utf32be_bytes_to_utf8(const uint8_t* data, size_t size, std::string &out);

// Convert a std::u16string (UTF-16) to UTF-8 string. Handles surrogate pairs.
// Returns true on success, false on invalid surrogate sequences.
bool utf16_string_to_utf8(const std::u16string &in, std::string &out);
