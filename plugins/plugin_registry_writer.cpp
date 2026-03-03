#include "plugins/plugin_registry_writer.h"

namespace hacpack::plugins {

bool serialize_plugin_refs(const std::vector<PluginRef> &refs, std::vector<std::uint8_t> &out, std::string &err)
{
    out.clear();
    if (refs.size() > 0xFFFFFFFFULL) {
        err = "Too many plugin refs";
        return false;
    }
    append_u32_le(out, static_cast<std::uint32_t>(refs.size()));

    for (const auto &r : refs) {
        if (r.name.size() > 0xFFFF) {
            err = "Plugin name too long: " + r.name;
            return false;
        }
        append_u16_le(out, static_cast<std::uint16_t>(r.name.size()));
        out.insert(out.end(), r.name.begin(), r.name.end());
        append_u32_le(out, r.file_id);
    }

    return true;
}

} // namespace hacpack::plugins

