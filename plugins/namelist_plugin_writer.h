#pragma once

#include <vector>

#include "hacpack_core.h"

namespace hacpack::plugins {

std::vector<std::uint8_t> build_namelist_payload(const std::vector<DataInput> &files);

} // namespace hacpack::plugins

