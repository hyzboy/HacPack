#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

struct HacPackEntry {
    // Stored filename from the info block as plain bytes (ANSI)
    std::string name;
    uint32_t size = 0;
    uint32_t offset = 0; // relative to start of data section
};

class HacPackIndex {
public:
    HacPackIndex();

    void clear();

    // Number of entries in the pack
    size_t file_count() const;

    // Get entries
    const std::vector<HacPackEntry>& entries() const;

    uint64_t info_size() const;
    uint64_t data_start() const;

private:
    // Internal population: loader will write directly into m_entries via friendship
    void set_info_size(uint64_t s);
    void set_data_start(uint64_t s);

    std::vector<HacPackEntry> m_entries;
    uint64_t m_info_size = 0;
    uint64_t m_data_start = 0; // file offset where data section begins

    // Allow IO loader to populate the index
    friend bool load_hacpack_index(const std::string &path, HacPackIndex &index, std::string &err);
};

// File I/O helpers are provided in pack_reader_io.h / .cpp

