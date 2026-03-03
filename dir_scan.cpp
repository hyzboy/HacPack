#include "dir_scan.h"
#include <filesystem>
#include <system_error>

bool collect_files_from_directory(const std::string &dir, std::vector<std::pair<std::string, std::string>> &out, std::string &err)
{
    out.clear();
    std::error_code ec;
    std::filesystem::path root(dir);
    if (!std::filesystem::exists(root, ec)) { err = "Directory does not exist: " + dir; return false; }
    if (!std::filesystem::is_directory(root, ec)) { err = "Not a directory: " + dir; return false; }

    for (auto it = std::filesystem::recursive_directory_iterator(root, std::filesystem::directory_options::skip_permission_denied, ec);
         it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) continue;
        const auto &entry = *it;
        if (!entry.is_regular_file(ec) || ec) continue;
        std::string disk_path = entry.path().string();
        std::string stored;
        std::error_code ec2;
        try {
            stored = std::filesystem::relative(entry.path(), root, ec2).generic_string();
        } catch (...) {
            ec2 = std::make_error_code(std::errc::io_error);
        }
        if (ec2) {
            stored = entry.path().filename().generic_string();
        }
        out.emplace_back(disk_path, stored);
    }

    if (out.empty()) {
        err = "No files found in directory: " + dir;
        return false;
    }
    return true;
}
