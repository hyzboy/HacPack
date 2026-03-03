#include "plugins/namelist_plugin_reader.h"

namespace hacpack::plugins {

bool init_namelist_plugin(const LoadedPack &pack, std::uint32_t file_id, PluginState &state, std::string &err)
{
    std::vector<std::uint8_t> payload;
    if (!read_pack_entry_data(pack, file_id, payload, err)) return false;

    std::size_t pos = 0;
    std::uint32_t count = 0;
    if (!read_u32_le(payload, pos, count)) {
        err = "NameList plugin payload corrupted (count)";
        return false;
    }

    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint32_t id = 0;
        std::uint32_t sz = 0;
        std::uint16_t name_len = 0;
        if (!read_u32_le(payload, pos, id) || !read_u32_le(payload, pos, sz) || !read_u16_le(payload, pos, name_len)) {
            err = "NameList plugin payload corrupted (entry header)";
            return false;
        }
        if (pos + name_len > payload.size()) {
            err = "NameList plugin payload corrupted (name bounds)";
            return false;
        }
        NameInfo info;
        info.id = id;
        info.size = sz;
        info.name.assign(reinterpret_cast<const char*>(&payload[pos]), name_len);
        pos += name_len;
        state.name_by_id[id] = std::move(info);
    }

    return true;
}

} // namespace hacpack::plugins

