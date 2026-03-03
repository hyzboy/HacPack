#include "plugins/plugin_registry_reader.h"

#include <functional>
#include <unordered_map>

#include "plugins/crc32_plugin_reader.h"
#include "plugins/namelist_plugin_reader.h"

namespace hacpack::plugins {

bool parse_plugin_refs(const std::vector<std::uint8_t> &buf, std::vector<PluginRef> &refs, std::string &err)
{
    refs.clear();
    std::size_t pos = 0;
    std::uint32_t count = 0;
    if (!read_u32_le(buf, pos, count)) {
        err = "Plugin KV corrupted (count)";
        return false;
    }

    refs.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint16_t name_len = 0;
        if (!read_u16_le(buf, pos, name_len)) {
            err = "Plugin KV corrupted (name len)";
            return false;
        }
        if (pos + name_len + 4 > buf.size()) {
            err = "Plugin KV corrupted (entry bounds)";
            return false;
        }

        PluginRef ref;
        ref.name.assign(reinterpret_cast<const char*>(&buf[pos]), name_len);
        pos += name_len;
        if (!read_u32_le(buf, pos, ref.file_id)) {
            err = "Plugin KV corrupted (file id)";
            return false;
        }
        refs.push_back(std::move(ref));
    }
    return true;
}

bool load_plugins_from_id0(const LoadedPack &pack, PluginState &state, std::string &err)
{
    state = PluginState{};
    state.metadata_file_ids.insert(0);

    std::vector<std::uint8_t> id0_blob;
    if (!read_pack_entry_data(pack, 0, id0_blob, err)) return false;

    std::vector<PluginRef> refs;
    if (!parse_plugin_refs(id0_blob, refs, err)) return false;

    using PluginInit = std::function<bool(const LoadedPack&, std::uint32_t, PluginState&, std::string&)>;
    std::unordered_map<std::string, PluginInit> registry;
    registry[kPluginNameList] = init_namelist_plugin;
    registry[kPluginCRC32] = init_crc32_plugin;

    for (const auto &ref : refs) {
        state.plugin_file_by_name[ref.name] = ref.file_id;
        state.metadata_file_ids.insert(ref.file_id);

        auto it = registry.find(ref.name);
        if (it == registry.end()) {
            continue;
        }
        if (!it->second(pack, ref.file_id, state, err)) {
            return false;
        }
    }

    return true;
}

} // namespace hacpack::plugins

