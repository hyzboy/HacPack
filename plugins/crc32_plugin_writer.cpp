#include "plugins/crc32_plugin_writer.h"

namespace hacpack::plugins {

std::vector<std::uint8_t> build_crc32_payload(const std::vector<DataInput> &files)
{
    std::vector<std::uint8_t> out;
    append_u32_le(out, static_cast<std::uint32_t>(files.size()));
    for (const auto &f : files) {
        append_u32_le(out, f.id);
        append_u32_le(out, f.crc32);
    }
    return out;
}

} // namespace hacpack::plugins

