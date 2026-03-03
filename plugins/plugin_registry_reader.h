#pragma once

#include <string>
#include <vector>

#include "hacpack_core.h"

namespace hacpack::plugins {

inline constexpr const char* kPluginNameList = "NameList";
inline constexpr const char* kPluginCRC32 = "CRC32";

bool parse_plugin_refs(const std::vector<std::uint8_t> &buf, std::vector<PluginRef> &refs, std::string &err);
bool load_plugins_from_id0(const LoadedPack &pack, PluginState &state, std::string &err);

} // namespace hacpack::plugins

