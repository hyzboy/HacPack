#pragma once

#include "pack_reader.h"
#include <string>
#include <vector>

// Load an index from a pack file into HacPackIndex. Returns true on success and sets err on failure.
bool load_hacpack_index(const std::string &path, HacPackIndex &index, std::string &err);

// Read file data by index entry into memory. Returns true on success and fills out buffer.
bool read_hacpack_entry_data(const std::string &path, const HacPackEntry &entry, std::vector<uint8_t> &out, std::string &err);

// Extract entry to file path
bool extract_hacpack_entry_to_file(const std::string &path, const HacPackEntry &entry, const std::string &out_path, std::string &err);

