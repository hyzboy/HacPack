#pragma once

#include <string>

#include "hacpack_core.h"

namespace hacpack::plugins {

bool init_crc32_plugin(const LoadedPack &pack, std::uint32_t file_id, PluginState &state, std::string &err);

} // namespace hacpack::plugins

