#if !defined(_WIN32)
#include "encoding.h"
#include "utf_conv.h"
#include <vector>
#include <cstring>
#include <fstream>
#include <iterator>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

bool read_text_file_as_utf8(const std::string &path, std::string &out)
{
    // Read raw bytes
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::vector<char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (data.size() >= 3 && static_cast<unsigned char>(data[0]) == 0xEF && static_cast<unsigned char>(data[1]) == 0xBB && static_cast<unsigned char>(data[2]) == 0xBF) {
        out.assign(data.data() + 3, data.size() - 3);
        return true;
    }
    if (data.size() >= 4 && static_cast<unsigned char>(data[0]) == 0xFF && static_cast<unsigned char>(data[1]) == 0xFE && static_cast<unsigned char>(data[2]) == 0x00 && static_cast<unsigned char>(data[3]) == 0x00) {
        // UTF-32 LE BOM
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.data() + 4);
        size_t sz = data.size() - 4;
#ifdef HAVE_ICONV
        iconv_t cd = iconv_open("UTF-8", "UTF-32LE");
        if (cd != (iconv_t)-1) {
            size_t in_bytes_left = sz;
            char *in_ptr = const_cast<char*>(reinterpret_cast<const char*>(bytes));
            std::vector<char> outbuf((in_bytes_left + 1) * 4);
            char *out_ptr = outbuf.data();
            size_t out_bytes_left = outbuf.size();
            size_t res = iconv(cd, &in_ptr, &in_bytes_left, &out_ptr, &out_bytes_left);
            iconv_close(cd);
            if (res != (size_t)-1) {
                out.assign(outbuf.data(), outbuf.size() - out_bytes_left);
                return true;
            }
        }
#endif
        if (utf32le_bytes_to_utf8(bytes, sz, out)) return true;
        return false;
    }
    if (data.size() >= 4 && static_cast<unsigned char>(data[0]) == 0x00 && static_cast<unsigned char>(data[1]) == 0x00 && static_cast<unsigned char>(data[2]) == 0xFE && static_cast<unsigned char>(data[3]) == 0xFF) {
        // UTF-32 BE BOM
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.data() + 4);
        size_t sz = data.size() - 4;
#ifdef HAVE_ICONV
        iconv_t cd = iconv_open("UTF-8", "UTF-32BE");
        if (cd != (iconv_t)-1) {
            size_t in_bytes_left = sz;
            char *in_ptr = const_cast<char*>(reinterpret_cast<const char*>(bytes));
            std::vector<char> outbuf((in_bytes_left + 1) * 4);
            char *out_ptr = outbuf.data();
            size_t out_bytes_left = outbuf.size();
            size_t res = iconv(cd, &in_ptr, &in_bytes_left, &out_ptr, &out_bytes_left);
            iconv_close(cd);
            if (res != (size_t)-1) {
                out.assign(outbuf.data(), outbuf.size() - out_bytes_left);
                return true;
            }
        }
#endif
        if (utf32be_bytes_to_utf8(bytes, sz, out)) return true;
        return false;
    }
    if (data.size() >= 2 && (unsigned char)data[0] == 0xFF && (unsigned char)data[1] == 0xFE) {
        // UTF-16 LE
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.data() + 2);
        size_t sz = data.size() - 2;
#ifdef HAVE_ICONV
        iconv_t cd = iconv_open("UTF-8", "UTF-16LE");
        if (cd != (iconv_t)-1) {
            size_t in_bytes_left = sz;
            char *in_ptr = const_cast<char*>(reinterpret_cast<const char*>(bytes));
            std::vector<char> outbuf((in_bytes_left + 1) * 2);
            char *out_ptr = outbuf.data();
            size_t out_bytes_left = outbuf.size();
            size_t res = iconv(cd, &in_ptr, &in_bytes_left, &out_ptr, &out_bytes_left);
            iconv_close(cd);
            if (res != (size_t)-1) {
                out.assign(outbuf.data(), outbuf.size() - out_bytes_left);
                return true;
            }
            // fallthrough to manual converter
        }
#endif
        if (utf16le_bytes_to_utf8(bytes, sz, out)) return true;
        return false;
    }
    if (data.size() >= 2 && (unsigned char)data[0] == 0xFE && (unsigned char)data[1] == 0xFF) {
        // UTF-16 BE
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.data() + 2);
        size_t sz = data.size() - 2;
#ifdef HAVE_ICONV
        iconv_t cd = iconv_open("UTF-8", "UTF-16BE");
        if (cd != (iconv_t)-1) {
            size_t in_bytes_left = sz;
            char *in_ptr = const_cast<char*>(reinterpret_cast<const char*>(bytes));
            std::vector<char> outbuf((in_bytes_left + 1) * 2);
            char *out_ptr = outbuf.data();
            size_t out_bytes_left = outbuf.size();
            size_t res = iconv(cd, &in_ptr, &in_bytes_left, &out_ptr, &out_bytes_left);
            iconv_close(cd);
            if (res != (size_t)-1) {
                out.assign(outbuf.data(), outbuf.size() - out_bytes_left);
                return true;
            }
            // fallthrough to manual converter
        }
#endif
        if (utf16be_bytes_to_utf8(bytes, sz, out)) return true;
        return false;
    }

    // No BOM -> assume UTF-8 on POSIX
    out.assign(data.data(), data.size());
    return true;
}
#endif
