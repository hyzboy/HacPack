#pragma once

#include <string>

// Read a text file and return its contents as UTF-8 in 'out'.
// Detects BOM for UTF-8/UTF-16LE/UTF-16BE. If no BOM:
// - on Windows assumes ANSI (CP_ACP)
// - on POSIX assumes UTF-8
// Returns true on success.
bool read_text_file_as_utf8(const std::string &path, std::string &out);
