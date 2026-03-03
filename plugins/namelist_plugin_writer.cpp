#include "plugins/namelist_plugin_writer.h"

namespace hacpack::plugins {

std::vector<std::uint8_t> build_namelist_payload(const std::vector<DataInput> &files)
{
    std::vector<std::uint8_t> out;
    append_u32_le(out, static_cast<std::uint32_t>(files.size()));
    for (const auto &f : files) {
        append_u32_le(out, f.id);
        append_u32_le(out, static_cast<std::uint32_t>(f.data.size()));
        append_u16_le(out, static_cast<std::uint16_t>(f.name.size()));
        out.insert(out.end(), f.name.begin(), f.name.end());
    }
    return out;
}

} // namespace hacpack::plugins

