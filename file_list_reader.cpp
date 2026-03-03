#include "file_list_reader.h"

#include <sstream>
#include <algorithm>

#include "encoding.h"

static std::string trim(const std::string &s)
{
    auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

bool read_file_list(const std::string &list_path, std::vector<std::string> &out_files, std::string &err)
{
    std::string list_content_utf8;
    if (!read_text_file_as_utf8(list_path, list_content_utf8)) {
        err = "Failed to read or decode list file: " + list_path;
        return false;
    }

    std::istringstream iss(list_content_utf8);
    std::string line;
    while (std::getline(iss, line)) {
        std::string s = trim(line);
        if (s.empty()) continue;
        if (!s.empty() && s[0] == '#') continue; // comment
        out_files.push_back(s);
    }

    if (out_files.empty()) {
        err = "No files listed in " + list_path;
        return false;
    }
    return true;
}
