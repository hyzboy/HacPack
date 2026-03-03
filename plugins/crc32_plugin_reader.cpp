#include "plugins/crc32_plugin_reader.h"

namespace hacpack::plugins {

bool init_crc32_plugin(const LoadedPack &pack, std::uint32_t file_id, PluginState &state, std::string &err)
{
    std::vector<std::uint8_t> payload;
    if (!read_pack_entry_data(pack, file_id, payload, err)) return false;

    std::size_t pos = 0;
    std::uint32_t count = 0;
    if (!read_u32_le(payload, pos, count)) {
        err = "CRC32 plugin payload corrupted (count)";
        return false;
    }

    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint32_t id = 0;
        std::uint32_t crc = 0;
        if (!read_u32_le(payload, pos, id) || !read_u32_le(payload, pos, crc)) {
            err = "CRC32 plugin payload corrupted (entry)";
            return false;
        }
        state.crc32_by_id[id] = crc;
    }

    return true;
}

} // namespace hacpack::plugins

