#include "pack_reader_io.h"
#include "pack_reader.h"
#include "utf_conv.h"
#include "hacpack_format.h"

#include <fstream>
#include <cstring>

bool load_hacpack_index(const std::string &path, HacPackIndex &index, std::string &err)
{
    index.clear();
    std::ifstream in(path, std::ios::binary);
    if (!in) { err = "Failed to open pack file: " + path; return false; }

    char magic[hacpack_format::kMagicSize];
    in.read(magic, static_cast<std::streamsize>(hacpack_format::kMagicSize));
    if (in.gcount() != static_cast<std::streamsize>(hacpack_format::kMagicSize)) { err = "Failed to read pack header"; return false; }
    if (std::memcmp(magic, hacpack_format::kMagic, hacpack_format::kMagicSize) != 0) { err = "Invalid pack magic"; return false; }

    uint8_t b[4];
    in.read(reinterpret_cast<char*>(b), 4);
    if (in.gcount() != 4) { err = "Failed to read info size"; return false; }
    uint32_t info_size = hacpack_format::read_u32_le_4(b);

    std::vector<uint8_t> info(info_size);
    in.read(reinterpret_cast<char*>(info.data()), static_cast<std::streamsize>(info_size));
    if (static_cast<size_t>(in.gcount()) != info_size) { err = "Failed to read info block"; return false; }

    size_t pos = 0;
    auto read_u32 = [&](uint32_t &out) -> bool {
        return hacpack_format::read_u32_le(info, pos, out);
    };
    uint32_t version = 0;
    if (!read_u32(version)) { err = "Info block corrupted (version)"; return false; }
    uint32_t file_count = 0;
    if (!read_u32(file_count)) { err = "Info block corrupted (file_count)"; return false; }

    // Read name lengths block (file_count bytes)
    if (pos + file_count > info.size()) { err = "Info block corrupted (name lengths)"; return false; }
    std::vector<uint8_t> name_lengths(file_count);
    for (uint32_t i = 0; i < file_count; ++i) name_lengths[i] = info[pos++];

    index.m_entries.reserve(file_count);

    // Read all names as raw bytes, each followed by a NUL terminator
    for (uint32_t i = 0; i < file_count; ++i) {
        uint32_t len = name_lengths[i];
        if (pos + len + 1 > info.size()) { err = "Info block corrupted (names area)"; return false; }
        std::string name;
        if (len > 0) {
            name.assign(reinterpret_cast<const char*>(&info[pos]), len);
        }
        pos += len;
        if (info[pos++] != 0) { err = "Info block corrupted (missing NUL after name)"; return false; }

        HacPackEntry e;
        e.name = std::move(name);
        index.m_entries.push_back(std::move(e));
    }

    // Read metadata: first all data_offsets, then all data_sizes
    std::vector<uint32_t> offsets(file_count, 0);
    for (uint32_t i = 0; i < file_count; ++i) {
        if (!read_u32(offsets[i])) { err = "Info block corrupted (offsets)"; return false; }
    }
    for (uint32_t i = 0; i < file_count; ++i) {
        uint32_t sz = 0;
        if (!read_u32(sz)) { err = "Info block corrupted (sizes)"; return false; }
        index.m_entries[i].size = sz;
        index.m_entries[i].offset = offsets[i];
    }

    index.set_info_size(info_size);
    index.set_data_start(hacpack_format::data_start_offset(info_size));
    return true;
}

bool read_hacpack_entry_data(const std::string &path, const HacPackEntry &entry, std::vector<uint8_t> &out, std::string &err)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) { err = "Failed to open pack for reading: " + path; return false; }
    // compute data start by reading header again
    // seek to info size
    in.seekg(static_cast<std::streamoff>(hacpack_format::kInfoSizeOffset), std::ios::beg);
    uint8_t b[4];
    in.read(reinterpret_cast<char*>(b), 4);
    if (in.gcount() != 4) { err = "Failed to read info size"; return false; }
    uint32_t info_size = hacpack_format::read_u32_le_4(b);
    std::uint64_t data_start = hacpack_format::data_start_offset(info_size);
    in.seekg(static_cast<std::streamoff>(data_start + entry.offset), std::ios::beg);
    out.resize(entry.size);
    in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(entry.size));
    if (in.gcount() != static_cast<std::streamsize>(entry.size)) { err = "Failed to read file data from pack"; return false; }
    return true;
}

bool extract_hacpack_entry_to_file(const std::string &path, const HacPackEntry &entry, const std::string &out_path, std::string &err)
{
    std::vector<uint8_t> data;
    if (!read_hacpack_entry_data(path, entry, data, err)) return false;
    std::ofstream out(out_path, std::ios::binary);
    if (!out) { err = "Failed to create output file: " + out_path; return false; }
    if (!data.empty()) out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return true;
}

