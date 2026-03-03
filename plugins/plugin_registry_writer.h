#pragma once

#include <string>
#include <vector>

#include "hacpack_core.h"

namespace hacpack::plugins {

bool serialize_plugin_refs(const std::vector<PluginRef> &refs, std::vector<std::uint8_t> &out, std::string &err);

} // namespace hacpack::plugins

