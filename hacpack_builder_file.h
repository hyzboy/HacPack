#pragma once

#include "hacpack_builder.h"

#include <string>

// Helpers that perform file-based operations using HacPackBuilder without
// embedding file I/O into the core builder class.

// Add a file from disk to the builder. 'file_path' is the path on disk,
// 'stored_name' is the name recorded inside the pack (if empty, uses file_path).
// Returns true on success, false and sets err on failure.
bool add_file_to_builder(HacPackBuilder &builder, const std::string &file_path, const std::string &stored_name, std::string &err);

// Convenience overload: store under the same name as on disk.
bool add_file_to_builder(HacPackBuilder &builder, const std::string &file_path, std::string &err);

