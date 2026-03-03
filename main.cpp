#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "dir_scan.h"
#include "hacpack_core.h"
#include "plugins/crc32_plugin_writer.h"
#include "plugins/namelist_plugin_writer.h"
#include "plugins/plugin_registry_reader.h"
#include "plugins/plugin_registry_writer.h"

namespace {

using namespace hacpack;

void print_usage(const char *exe)
{
    std::cout
        << "HacPack CLI\n"
        << "Usage:\n"
        << "  " << exe << " pack <directory> <output.hac>\n"
        << "  " << exe << " unpack <input.hac> <output_directory>\n"
        << "  " << exe << " list <input.hac> [--hash]\n"
        << "  " << exe << " compare <input.hac> <directory>\n";
}

int cmd_pack(const std::string &dir_path, const std::string &out_path)
{
    std::vector<std::pair<std::string, std::string>> file_pairs;
    std::string err;
    if (!collect_files_from_directory(dir_path, file_pairs, err)) {
        std::cerr << err << "\n";
        return 1;
    }

    std::sort(file_pairs.begin(), file_pairs.end(), [](const auto &a, const auto &b) {
        return a.second < b.second;
    });

    std::vector<DataInput> files;
    files.reserve(file_pairs.size());

    std::uint32_t next_id = 1;
    for (const auto &pair : file_pairs) {
        DataInput in;
        in.id = next_id++;
        in.name = normalize_name(pair.second);
        if (!read_file_bytes(pair.first, in.data, err)) {
            std::cerr << err << "\n";
            return 1;
        }
        in.crc32 = crc32_compute(in.data);
        files.push_back(std::move(in));
    }

    const std::uint32_t namelist_id = next_id++;
    const std::uint32_t crc32_id = next_id++;

    std::vector<std::pair<std::uint32_t, std::vector<std::uint8_t>>> id_data;
    id_data.reserve(files.size() + 3);

    for (const auto &f : files) {
        id_data.emplace_back(f.id, f.data);
    }
    id_data.emplace_back(namelist_id, plugins::build_namelist_payload(files));
    id_data.emplace_back(crc32_id, plugins::build_crc32_payload(files));

    std::vector<PluginRef> refs;
    refs.push_back(PluginRef{plugins::kPluginNameList, namelist_id});
    refs.push_back(PluginRef{plugins::kPluginCRC32, crc32_id});

    std::vector<std::uint8_t> id0_blob;
    if (!plugins::serialize_plugin_refs(refs, id0_blob, err)) {
        std::cerr << err << "\n";
        return 1;
    }
    id_data.emplace_back(0, std::move(id0_blob));

    if (!write_pack(out_path, id_data, err)) {
        std::cerr << err << "\n";
        return 1;
    }

    std::cout << "Packed " << files.size() << " files into " << out_path
              << " (version=V2, plugins=[NameList,CRC32])\n";
    return 0;
}

int cmd_unpack(const std::string &pack_path, const std::string &out_dir)
{
    std::string err;
    LoadedPack pack;
    if (!load_pack(pack_path, pack, err)) {
        std::cerr << err << "\n";
        return 1;
    }

    PluginState state;
    if (!plugins::load_plugins_from_id0(pack, state, err)) {
        std::cerr << err << "\n";
        return 1;
    }

    std::filesystem::path base(out_dir);
    std::error_code ec;
    std::filesystem::create_directories(base, ec);
    if (ec) {
        std::cerr << "Failed to create output directory: " << out_dir << "\n";
        return 1;
    }

    std::size_t unpacked_count = 0;
    for (const auto &entry : pack.entries) {
        if (state.metadata_file_ids.find(entry.id) != state.metadata_file_ids.end()) continue;

        auto name_it = state.name_by_id.find(entry.id);
        std::string stored_name = (name_it != state.name_by_id.end()) ? name_it->second.name : ("id_" + std::to_string(entry.id) + ".bin");

        std::filesystem::path rel(stored_name);
        if (stored_name.empty() || rel.is_absolute() || contains_parent_ref(rel)) {
            std::cerr << "Unsafe entry path in pack: " << stored_name << "\n";
            return 1;
        }

        std::filesystem::path target = base / rel;
        std::filesystem::create_directories(target.parent_path(), ec);
        if (ec) {
            std::cerr << "Failed to create directory: " << target.parent_path().string() << "\n";
            return 1;
        }

        std::vector<std::uint8_t> data;
        if (!read_pack_entry_data(pack, entry.id, data, err)) {
            std::cerr << err << "\n";
            return 1;
        }

        std::ofstream out(target.string(), std::ios::binary | std::ios::trunc);
        if (!out) {
            std::cerr << "Failed to create output file: " << target.string() << "\n";
            return 1;
        }
        if (!data.empty()) out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        ++unpacked_count;
    }

    std::cout << "Unpacked " << unpacked_count << " files to " << out_dir << "\n";
    return 0;
}

int cmd_list(const std::string &pack_path, bool show_hash)
{
    std::string err;
    LoadedPack pack;
    if (!load_pack(pack_path, pack, err)) {
        std::cerr << err << "\n";
        return 1;
    }

    PluginState state;
    if (!plugins::load_plugins_from_id0(pack, state, err)) {
        std::cerr << err << "\n";
        return 1;
    }

    std::size_t user_count = 0;
    for (const auto &entry : pack.entries) {
        if (state.metadata_file_ids.find(entry.id) == state.metadata_file_ids.end()) ++user_count;
    }

    std::cout << "version=V2"
              << ", file_count=" << user_count
              << ", data_start=" << pack.data_start << "\n";

    if (show_hash) {
        std::cout << "id\toffset\tsize\thash(CRC32)\tname\n";
    } else {
        std::cout << "id\toffset\tsize\tname\n";
    }

    for (const auto &entry : pack.entries) {
        if (state.metadata_file_ids.find(entry.id) != state.metadata_file_ids.end()) continue;

        auto name_it = state.name_by_id.find(entry.id);
        std::string name = (name_it != state.name_by_id.end()) ? name_it->second.name : ("id_" + std::to_string(entry.id) + ".bin");

        if (!show_hash) {
            std::cout << entry.id << '\t' << entry.offset << '\t' << entry.size << '\t' << name << "\n";
            continue;
        }

        std::uint32_t crc = 0;
        auto crc_it = state.crc32_by_id.find(entry.id);
        if (crc_it != state.crc32_by_id.end()) {
            crc = crc_it->second;
        } else {
            std::vector<std::uint8_t> data;
            if (!read_pack_entry_data(pack, entry.id, data, err)) {
                std::cerr << err << "\n";
                return 1;
            }
            crc = crc32_compute(data);
        }

        std::cout << entry.id << '\t' << entry.offset << '\t' << entry.size << '\t' << hex_u32(crc) << '\t' << name << "\n";
    }

    return 0;
}

int cmd_compare(const std::string &pack_path, const std::string &dir_path)
{
    std::string err;
    LoadedPack pack;
    if (!load_pack(pack_path, pack, err)) {
        std::cerr << err << "\n";
        return 1;
    }

    PluginState state;
    if (!plugins::load_plugins_from_id0(pack, state, err)) {
        std::cerr << err << "\n";
        return 1;
    }

    std::vector<std::pair<std::string, std::string>> disk_files;
    if (!collect_files_from_directory(dir_path, disk_files, err)) {
        std::cerr << err << "\n";
        return 1;
    }

    std::unordered_map<std::string, std::uint32_t> dir_hash;
    std::unordered_map<std::string, std::uint32_t> dir_size;
    for (const auto &pair : disk_files) {
        std::vector<std::uint8_t> data;
        if (!read_file_bytes(pair.first, data, err)) {
            std::cerr << err << "\n";
            return 1;
        }
        std::string name = normalize_name(pair.second);
        dir_size[name] = static_cast<std::uint32_t>(data.size());
        dir_hash[name] = crc32_compute(data);
    }

    std::unordered_map<std::string, std::uint32_t> pack_hash;
    std::unordered_map<std::string, std::uint32_t> pack_size;
    for (const auto &entry : pack.entries) {
        if (state.metadata_file_ids.find(entry.id) != state.metadata_file_ids.end()) continue;

        auto name_it = state.name_by_id.find(entry.id);
        if (name_it == state.name_by_id.end()) {
            std::cerr << "Missing name mapping for file id: " << entry.id << "\n";
            return 1;
        }

        std::string name = normalize_name(name_it->second.name);
        pack_size[name] = entry.size;

        auto crc_it = state.crc32_by_id.find(entry.id);
        if (crc_it != state.crc32_by_id.end()) {
            pack_hash[name] = crc_it->second;
        } else {
            std::vector<std::uint8_t> data;
            if (!read_pack_entry_data(pack, entry.id, data, err)) {
                std::cerr << err << "\n";
                return 1;
            }
            pack_hash[name] = crc32_compute(data);
        }
    }

    bool same = true;
    std::size_t only_in_pack = 0;
    std::size_t only_in_dir = 0;
    std::size_t changed = 0;

    for (const auto &kv : pack_hash) {
        auto it = dir_hash.find(kv.first);
        if (it == dir_hash.end()) {
            same = false;
            ++only_in_pack;
            std::cout << "ONLY_IN_PACK\t" << kv.first << "\n";
            continue;
        }

        if (kv.second != it->second || pack_size[kv.first] != dir_size[kv.first]) {
            same = false;
            ++changed;
            std::cout << "DIFF\t" << kv.first
                      << "\tpack_size=" << pack_size[kv.first]
                      << "\tdir_size=" << dir_size[kv.first]
                      << "\tpack_hash=" << hex_u32(kv.second)
                      << "\tdir_hash=" << hex_u32(it->second)
                      << "\n";
        }
    }

    for (const auto &kv : dir_hash) {
        if (pack_hash.find(kv.first) == pack_hash.end()) {
            same = false;
            ++only_in_dir;
            std::cout << "ONLY_IN_DIR\t" << kv.first << "\n";
        }
    }

    if (same) {
        std::cout << "MATCH\tpack and directory are identical\n";
        return 0;
    }

    std::cout << "MISMATCH\tonly_in_pack=" << only_in_pack
              << "\tonly_in_dir=" << only_in_dir
              << "\tcontent_diff=" << changed << "\n";
    return 2;
}

} // namespace

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "pack") {
        if (argc < 4) {
            print_usage(argv[0]);
            return 1;
        }
        return cmd_pack(argv[2], argv[3]);
    }

    if (cmd == "unpack") {
        if (argc < 4) {
            print_usage(argv[0]);
            return 1;
        }
        return cmd_unpack(argv[2], argv[3]);
    }

    if (cmd == "list") {
        if (argc < 3) {
            print_usage(argv[0]);
            return 1;
        }
        bool show_hash = false;
        for (int i = 3; i < argc; ++i) {
            if (std::string(argv[i]) == "--hash") show_hash = true;
        }
        return cmd_list(argv[2], show_hash);
    }

    if (cmd == "compare") {
        if (argc < 4) {
            print_usage(argv[0]);
            return 1;
        }
        return cmd_compare(argv[2], argv[3]);
    }

    print_usage(argv[0]);
    return 1;
}

