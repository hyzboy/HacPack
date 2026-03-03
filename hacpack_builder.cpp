#include "hacpack_builder.h"
#include "hacpack_format.h"

#include <limits>
#include <vector>
#include <memory>
#include <iostream>
#include <cstring>

HacPackBuilder::HacPackBuilder() = default;

void HacPackBuilder::clear() { m_entries.clear(); }

bool HacPackBuilder::empty() const { return m_entries.empty(); }

std::size_t HacPackBuilder::file_count() const { return m_entries.size(); }

bool HacPackBuilder::add_entry_from_buffer(const std::string &name, const std::vector<std::uint8_t> &data, std::string &err)
{
    if (name.empty()) { err = "Failed to convert name: empty"; return false; }
    if (data.size() > std::numeric_limits<std::uint32_t>::max()) {
        err = "Buffer too large for HacPack entry";
        return false;
    }

    std::cout<<"HacPackBuilder::add_entry_from_buffer("<<name<<", data of size "<<data.size()<<")"<<std::endl;

    return add_entry_internal(name, data, err);
}

bool HacPackBuilder::add_entry_from_buffer(const std::string &name, const void *data, std::uint32_t size, std::string &err)
{
    if (name.empty()) { err = "Failed to convert name: empty"; return false; }
    if (size == 0) {
        // allow empty entry
        std::vector<std::uint8_t> empty;
        return add_entry_internal(name, std::move(empty), err);
    }
    if (data == nullptr) { err = "Data pointer is null"; return false; }

    std::cout<<"HacPackBuilder::add_entry_from_buffer("<<name<<", raw pointer size "<<size<<")"<<std::endl;

    // Copy into owned vector
    std::vector<std::uint8_t> buf;
    buf.resize(size);
    std::memcpy(buf.data(), data, size);
    return add_entry_internal(name, std::move(buf), err);
}

bool HacPackBuilder::build_index(std::vector<std::uint8_t> &header, std::vector<std::uint32_t> &offsets, HacPackBuildResult &result, std::string &err) const
{
    if (m_entries.empty()) {
        err = "No entries added to HacPack";
        return false;
    }

    std::vector<std::uint8_t> info;
    info.reserve(16 * m_entries.size());

    hacpack_format::append_u32_le(info, hacpack_format::kVersion);
    hacpack_format::append_u32_le(info, static_cast<std::uint32_t>(m_entries.size()));

    // 1) Write all name lengths (uint8, not including the trailing NUL)
    std::vector<std::uint8_t> name_lengths;
    name_lengths.reserve(m_entries.size());
    for (const auto &entry : m_entries) {
        if (entry.name.size() > 0xFF) {
            err = "Filename too long (max 255 bytes): " + entry.name;
            return false;
        }
        name_lengths.push_back(static_cast<std::uint8_t>(entry.name.size()));
    }
    // append lengths block
    info.insert(info.end(), name_lengths.begin(), name_lengths.end());

    // 2) Write all names as raw bytes, each followed by a NUL terminator (\0)
    for (const auto &entry : m_entries) {
        if (!entry.name.empty())
            info.insert(info.end(), entry.name.begin(), entry.name.end());
        info.push_back(0); // NUL terminator
    }

    // 3) Compute offsets and total sizes
    offsets.clear();
    offsets.reserve(m_entries.size());
    std::uint64_t current_offset = 0;
    std::uint64_t total_data_size = 0;

    for (const auto &entry : m_entries) {
        total_data_size += entry.data.size();
        if (total_data_size > std::numeric_limits<std::uint32_t>::max()) {
            err = "Total data too large for 32-bit offsets/sizes";
            return false;
        }
    }

    for (const auto &entry : m_entries) {
        offsets.push_back(static_cast<std::uint32_t>(current_offset));
        current_offset += entry.data.size();
    }

    // 4) Append all data_offsets for all files, then all data_sizes for all files
    for (std::size_t i = 0; i < m_entries.size(); ++i) {
        hacpack_format::append_u32_le(info, offsets[i]);
    }
    for (std::size_t i = 0; i < m_entries.size(); ++i) {
        hacpack_format::append_u32_le(info, static_cast<std::uint32_t>(m_entries[i].data.size()));
    }

    if (info.size() > std::numeric_limits<std::uint32_t>::max()) {
        err = "Info block too large";
        return false;
    }

    header.clear();
    header.insert(header.end(), std::begin(hacpack_format::kMagic), std::end(hacpack_format::kMagic));
    hacpack_format::append_u32_le(header, static_cast<std::uint32_t>(info.size()));
    header.insert(header.end(), info.begin(), info.end());

    result.info_size = info.size();
    result.total_data_size = total_data_size;
    result.file_count = m_entries.size();
    return true;
}

bool HacPackBuilder::build_pack(HacPackWriter *writer, bool index_only, HacPackBuildResult &result, std::string &err) const
{
    if (!writer) { err = "Writer is null"; return false; }

    std::vector<std::uint8_t> header;
    std::vector<std::uint32_t> offsets;
    if (!build_index(header, offsets, result, err)) return false;

    if (!writer->write(header.data(), header.size(), err)) return false;
    if (index_only) return true;

    for (const auto &entry : m_entries) {
        if (!entry.data.empty()) {
            if (!writer->write(entry.data.data(), entry.data.size(), err)) return false;
        }
    }

    return true;
}

bool HacPackBuilder::add_entry_internal(std::string name, std::vector<std::uint8_t> data, std::string &err)
{
    if (name.empty()) { err = "Entry name cannot be empty"; return false; }
    if (data.size() > std::numeric_limits<std::uint32_t>::max()) { err = "Entry size exceeds limit"; return false; }
    m_entries.push_back(Entry{std::move(name), std::move(data)});
    return true;
}

bool HacPackBuilder::add_entry_internal(std::string name, const void *data, std::uint32_t size, std::string &err)
{
    if (name.empty()) { err = "Entry name cannot be empty"; return false; }
    if (size > std::numeric_limits<std::uint32_t>::max()) { err = "Entry size exceeds limit"; return false; }
    if (size == 0) {
        m_entries.push_back(Entry{std::move(name), {}});
        return true;
    }
    if (data == nullptr) { err = "Data pointer is null"; return false; }

    std::vector<std::uint8_t> buf;
    buf.resize(size);
    std::memcpy(buf.data(), data, size);
    m_entries.push_back(Entry{std::move(name), std::move(buf)});
    return true;
}

void write_string_list(HacPackBuilder *builder,const std::string &entry_name,const std::vector<std::string> &str_list,std::string &err)
{
    if (!builder) {
        err = "Builder is null";
        return;
    }

    if (entry_name.empty()) {
        err = "Entry name cannot be empty";
        return;
    }

    uint32_t str_count = static_cast<uint32_t>(str_list.size());

    // Compute total length of all strings (+1 for each NUL)
    std::uint32_t total_length = 0;
    for (const auto &s : str_list) {
        if (s.size() > 0xFF) {
            err = "String too long (max 255 bytes): " + s;
            return;
        }
        total_length += static_cast<std::uint32_t>(s.size()) + 1; // include NUL
    }

    // Build buffer: 4 bytes count, then str_count bytes of lengths, then strings with NULs
    std::vector<std::uint8_t> buf;
    buf.reserve(4 + str_count + total_length);

    // Append count (little-endian)
    hacpack_format::append_u32_le(buf, str_count);

    // Append lengths
    for (const auto &s : str_list) {
        buf.push_back(static_cast<std::uint8_t>(s.size()));
    }

    // Append strings followed by NUL
    for (const auto &s : str_list) {
        if (!s.empty()) {
            buf.insert(buf.end(), s.begin(), s.end());
        }
        buf.push_back(0);
    }

    // Add as a single entry
    if (!builder->add_entry_from_buffer(entry_name, buf, err)) {
        // err is already set by add_entry_from_buffer
        return;
    }
}

