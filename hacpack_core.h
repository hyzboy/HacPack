#pragma once

#include <cstdint>
#include <filesystem>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace hacpack {

inline constexpr std::uint32_t kVersion = 2;

struct CoreEntry {
    std::uint32_t id = 0;
    std::uint32_t offset = 0;
    std::uint32_t size = 0;
};

struct LoadedPack {
    std::string path;
    std::uint64_t data_start = 0;
    std::vector<CoreEntry> entries;
    std::unordered_map<std::uint32_t, CoreEntry> by_id;
};

struct PluginRef {
    std::string name;
    std::uint32_t file_id = 0;
};

struct NameInfo {
    std::uint32_t id = 0;
    std::uint32_t size = 0;
    std::string name;
};

struct PluginState {
    std::unordered_map<std::uint32_t, NameInfo> name_by_id;
    std::unordered_map<std::uint32_t, std::uint32_t> crc32_by_id;
    std::set<std::uint32_t> metadata_file_ids;
    std::unordered_map<std::string, std::uint32_t> plugin_file_by_name;
};

struct DataInput {
    std::uint32_t id = 0;
    std::string name;
    std::vector<std::uint8_t> data;
    std::uint32_t crc32 = 0;
};

void append_u16_le(std::vector<std::uint8_t> &buf, std::uint16_t v);
void append_u32_le(std::vector<std::uint8_t> &buf, std::uint32_t v);
bool read_u16_le(const std::vector<std::uint8_t> &buf, std::size_t &pos, std::uint16_t &out);
bool read_u32_le(const std::vector<std::uint8_t> &buf, std::size_t &pos, std::uint32_t &out);

std::string normalize_name(std::string name);
std::string hex_u32(std::uint32_t v);
std::uint32_t crc32_compute(const std::vector<std::uint8_t> &data);

bool read_file_bytes(const std::string &path, std::vector<std::uint8_t> &out, std::string &err);
bool contains_parent_ref(const std::filesystem::path &p);

bool write_pack(const std::string &out_path,
                   const std::vector<std::pair<std::uint32_t, std::vector<std::uint8_t>>> &id_data,
                   std::string &err);

bool load_pack(const std::string &path, LoadedPack &pack, std::string &err);
bool read_pack_entry_data(const LoadedPack &pack, std::uint32_t id, std::vector<std::uint8_t> &out, std::string &err);

} // namespace hacpack

