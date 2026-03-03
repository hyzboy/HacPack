#ifdef _WIN32
#include "encoding.h"
#include "utf_conv.h"
#include <windows.h>
#include <vector>
#include <fstream>
#include <iterator>

bool read_text_file_as_utf8(const std::string &path, std::string &out)
{
    // Read raw bytes
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    if (data.size() >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        // UTF-8 BOM
        out.assign(reinterpret_cast<char*>(data.data() + 3), data.size() - 3);
        return true;
    }
    if (data.size() >= 4 && data[0] == 0xFF && data[1] == 0xFE && data[2] == 0x00 && data[3] == 0x00) {
        // UTF-32 LE BOM
        const uint8_t* bytes = data.data() + 4;
        size_t sz = data.size() - 4;
        if (utf32le_bytes_to_utf8(bytes, sz, out)) return true;
        return false;
    }
    if (data.size() >= 4 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0xFE && data[3] == 0xFF) {
        // UTF-32 BE BOM
        const uint8_t* bytes = data.data() + 4;
        size_t sz = data.size() - 4;
        if (utf32be_bytes_to_utf8(bytes, sz, out)) return true;
        return false;
    }
    if (data.size() >= 2 && data[0] == 0xFF && data[1] == 0xFE) {
        // UTF-16 LE BOM
        const uint8_t* bytes = data.data() + 2;
        size_t sz = data.size() - 2;
        if (utf16le_bytes_to_utf8(bytes, sz, out)) return true;
        return false;
    }
    if (data.size() >= 2 && data[0] == 0xFE && data[1] == 0xFF) {
        // UTF-16 BE BOM
        const uint8_t* bytes = data.data() + 2;
        size_t sz = data.size() - 2;
        if (utf16be_bytes_to_utf8(bytes, sz, out)) return true;
        return false;
    }

    // No BOM -> assume ANSI (CP_ACP)
    int needed = MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()), nullptr, 0);
    if (needed <= 0) return false;
    std::wstring w;
    w.resize(needed);
    int rc = MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()), &w[0], needed);
    if (rc <= 0) return false;
    int out_needed = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (out_needed <= 0) return false;
    out.resize(out_needed);
    rc = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), &out[0], out_needed, nullptr, nullptr);
    return rc == out_needed;
}
#endif
