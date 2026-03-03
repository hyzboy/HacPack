#include "hacpack_core.h"
#include "hacpack_format.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace hacpack {

void append_u16_le(std::vector<std::uint8_t> &buf, std::uint16_t v)
{
    buf.push_back(static_cast<std::uint8_t>(v & 0xFF));
    buf.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
}

void append_u32_le(std::vector<std::uint8_t> &buf, std::uint32_t v)
{
    for (int i = 0; i < 4; ++i) {
        buf.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
    }
}

bool read_u16_le(const std::vector<std::uint8_t> &buf, std::size_t &pos, std::uint16_t &out)
{
    if (pos + 2 > buf.size()) return false;
    out = static_cast<std::uint16_t>(buf[pos] | (buf[pos + 1] << 8));
    pos += 2;
    return true;
}

bool read_u32_le(const std::vector<std::uint8_t> &buf, std::size_t &pos, std::uint32_t &out)
{
    if (pos + 4 > buf.size()) return false;
    out = static_cast<std::uint32_t>(buf[pos] | (buf[pos + 1] << 8) | (buf[pos + 2] << 16) | (buf[pos + 3] << 24));
    pos += 4;
    return true;
}

std::string normalize_name(std::string name)
{
    std::replace(name.begin(), name.end(), '\\', '/');
    return name;
}

std::string hex_u32(std::uint32_t v)
{
    std::ostringstream oss;
    oss << std::hex << std::setw(8) << std::setfill('0') << v;
    return oss.str();
}

std::uint32_t crc32_compute(const std::vector<std::uint8_t> &data)
{
    static std::array<std::uint32_t, 256> table = []() {
        std::array<std::uint32_t, 256> t{};
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int j = 0; j < 8; ++j) {
                if (c & 1) c = 0xEDB88320U ^ (c >> 1);
                else c >>= 1;
            }
            t[i] = c;
        }
        return t;
    }();

    std::uint32_t c = 0xFFFFFFFFU;
    for (std::uint8_t b : data) c = table[(c ^ b) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFU;
}

bool read_file_bytes(const std::string &path, std::vector<std::uint8_t> &out, std::string &err)
{
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) {
        err = "Failed to open file: " + path;
        return false;
    }

    auto size = in.tellg();
    if (size == static_cast<std::ifstream::pos_type>(-1)) {
        err = "Failed to determine file size: " + path;
        return false;
    }

    out.resize(static_cast<std::size_t>(size));
    in.seekg(0, std::ios::beg);
    if (!out.empty()) {
        in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(out.size()));
        if (!in) {
            err = "Failed to read file: " + path;
            return false;
        }
    }
    return true;
}

bool contains_parent_ref(const std::filesystem::path &p)
{
    for (const auto &part : p) {
        if (part == "..") return true;
    }
    return false;
}

bool write_pack(const std::string &out_path,
                   const std::vector<std::pair<std::uint32_t, std::vector<std::uint8_t>>> &id_data,
                   std::string &err)
{
    if (id_data.empty()) {
        err = "No entries to write";
        return false;
    }

    bool has_zero = false;
    std::uint64_t data_size_total = 0;
    std::vector<CoreEntry> entries;
    entries.reserve(id_data.size());

    std::uint32_t current_offset = 0;
    for (const auto &it : id_data) {
        const std::uint32_t id = it.first;
        const std::vector<std::uint8_t> &data = it.second;
        if (id == 0) has_zero = true;
        if (data.size() > 0xFFFFFFFFULL) {
            err = "Entry too large for 32-bit size";
            return false;
        }
        if (data_size_total + data.size() > 0xFFFFFFFFULL) {
            err = "Total data too large for 32-bit offsets/sizes";
            return false;
        }

        CoreEntry e;
        e.id = id;
        e.offset = current_offset;
        e.size = static_cast<std::uint32_t>(data.size());
        entries.push_back(e);

        current_offset = static_cast<std::uint32_t>(current_offset + e.size);
        data_size_total += data.size();
    }

    if (!has_zero) {
        err = "V2 pack requires metadata entry with id=0";
        return false;
    }

    std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        err = "Failed to open output file: " + out_path;
        return false;
    }

    out.write(hacpack_format::kMagic, static_cast<std::streamsize>(hacpack_format::kMagicSize));
    std::vector<std::uint8_t> header;
    append_u32_le(header, hacpack_format::kVersion);
    append_u32_le(header, static_cast<std::uint32_t>(entries.size()));
    for (const auto &e : entries) {
        append_u32_le(header, e.id);
        append_u32_le(header, e.offset);
        append_u32_le(header, e.size);
    }
    out.write(reinterpret_cast<const char*>(header.data()), static_cast<std::streamsize>(header.size()));

    for (const auto &it : id_data) {
        const auto &data = it.second;
        if (!data.empty()) {
            out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        }
    }

    if (!out) {
        err = "Failed while writing pack: " + out_path;
        return false;
    }
    return true;
}

bool load_pack(const std::string &path, LoadedPack &pack, std::string &err)
{
    pack = LoadedPack{};
    pack.path = path;

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        err = "Failed to open pack file: " + path;
        return false;
    }

    char magic[hacpack_format::kMagicSize];
    in.read(magic, static_cast<std::streamsize>(hacpack_format::kMagicSize));
    if (in.gcount() != static_cast<std::streamsize>(hacpack_format::kMagicSize)
        || std::memcmp(magic, hacpack_format::kMagic, hacpack_format::kMagicSize) != 0) {
        err = "Invalid pack magic";
        return false;
    }

    std::uint8_t b4[4];
    in.read(reinterpret_cast<char*>(b4), 4);
    if (in.gcount() != 4) {
        err = "Failed to read version";
        return false;
    }
    std::uint32_t version = static_cast<std::uint32_t>(b4[0] | (b4[1] << 8) | (b4[2] << 16) | (b4[3] << 24));
    if (version != hacpack_format::kVersion) {
        err = "Unsupported pack version (only V2 is supported)";
        return false;
    }

    in.read(reinterpret_cast<char*>(b4), 4);
    if (in.gcount() != 4) {
        err = "Failed to read entry_count";
        return false;
    }
    std::uint32_t entry_count = static_cast<std::uint32_t>(b4[0] | (b4[1] << 8) | (b4[2] << 16) | (b4[3] << 24));

    pack.entries.reserve(entry_count);
    for (std::uint32_t i = 0; i < entry_count; ++i) {
        std::uint8_t rec[12];
        in.read(reinterpret_cast<char*>(rec), 12);
        if (in.gcount() != 12) {
            err = "Failed to read core index record";
            return false;
        }

        CoreEntry e;
        e.id = static_cast<std::uint32_t>(rec[0] | (rec[1] << 8) | (rec[2] << 16) | (rec[3] << 24));
        e.offset = static_cast<std::uint32_t>(rec[4] | (rec[5] << 8) | (rec[6] << 16) | (rec[7] << 24));
        e.size = static_cast<std::uint32_t>(rec[8] | (rec[9] << 8) | (rec[10] << 16) | (rec[11] << 24));
        pack.entries.push_back(e);
        pack.by_id[e.id] = e;
    }

    pack.data_start = static_cast<std::uint64_t>(hacpack_format::kMagicSize) + 4 + 4 + static_cast<std::uint64_t>(entry_count) * 12ULL;
    if (pack.by_id.find(0) == pack.by_id.end()) {
        err = "V2 pack missing metadata entry id=0";
        return false;
    }
    return true;
}

bool read_pack_entry_data(const LoadedPack &pack, std::uint32_t id, std::vector<std::uint8_t> &out, std::string &err)
{
    auto it = pack.by_id.find(id);
    if (it == pack.by_id.end()) {
        err = "Entry id not found: " + std::to_string(id);
        return false;
    }

    std::ifstream in(pack.path, std::ios::binary);
    if (!in) {
        err = "Failed to open pack file: " + pack.path;
        return false;
    }

    const CoreEntry &e = it->second;
    in.seekg(static_cast<std::streamoff>(pack.data_start + e.offset), std::ios::beg);
    out.resize(e.size);
    if (e.size > 0) {
        in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(e.size));
        if (in.gcount() != static_cast<std::streamsize>(e.size)) {
            err = "Failed to read entry data";
            return false;
        }
    }
    return true;
}

} // namespace hacpack

