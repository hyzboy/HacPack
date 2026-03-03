#include "hacpack_builder_file.h"

#include <fstream>
#include <limits>
#include <vector>
#include <cstdint>

bool add_file_to_builder(HacPackBuilder &builder, const std::string &file_path, const std::string &stored_name, std::string &err)
{
    // Open file and read whole contents into memory
    std::ifstream in(file_path, std::ios::binary | std::ios::ate);
    if (!in) {
        err = "Failed to open input file: " + file_path;
        return false;
    }

    std::ifstream::pos_type pos = in.tellg();
    if (pos == static_cast<std::ifstream::pos_type>(-1)) {
        err = "Failed to determine size for file: " + file_path;
        return false;
    }

    std::uint64_t file_size = static_cast<std::uint64_t>(pos);
    if (file_size > std::numeric_limits<std::uint32_t>::max()) {
        err = "File too large (must fit in 32-bit size): " + file_path;
        return false;
    }

    std::vector<std::uint8_t> buffer;
    buffer.resize(static_cast<std::size_t>(file_size));

    in.seekg(0, std::ios::beg);
    if (file_size > 0) {
        in.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        if (!in) {
            err = "Failed to read input file: " + file_path;
            return false;
        }
    }
    in.close();

    std::string effective_name = stored_name.empty() ? file_path : stored_name;
    return builder.add_entry_from_buffer(effective_name, buffer, err);
}

bool add_file_to_builder(HacPackBuilder &builder, const std::string &file_path, std::string &err)
{
    return add_file_to_builder(builder, file_path, file_path, err);
}

