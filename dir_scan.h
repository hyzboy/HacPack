#pragma once

#include <string>
#include <vector>

// Enumerate files recursively under 'dir'. For each regular file found, append
// a pair (disk_path, stored_name) to 'out'. 'stored_name' is the path
// relative to 'dir' using forward slashes.
// Returns true on success. On failure, returns false and sets 'err'.
bool collect_files_from_directory(const std::string &dir, std::vector<std::pair<std::string, std::string>> &out, std::string &err);
