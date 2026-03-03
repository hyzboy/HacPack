#pragma once

#include <string>
#include <vector>

// Read a list of file paths (UTF-8 text file). Returns true on success and fills out_files.
// On error, returns false and sets err to a message.
bool read_file_list(const std::string &list_path, std::vector<std::string> &out_files, std::string &err);
