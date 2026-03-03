#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "pack_reader.h"

struct HacPackBuildResult
{
    std::size_t info_size=0;
    std::uint64_t total_data_size=0;
    std::size_t file_count=0;
};

// Abstract writer interface
class HacPackWriter
{
public:
    virtual ~HacPackWriter()=default;
    // Write 'size' bytes from data, return true on success, false and set err on failure
    virtual bool write(const std::uint8_t *data,std::size_t size,std::string &err)=0;
};

// Factory functions to create default writers. Implementations hidden in cpp
std::unique_ptr<HacPackWriter> create_vector_writer(std::vector<std::uint8_t> &out);
// Create a writer that writes directly to a file specified by path. Returns nullptr on failure to open file.
std::unique_ptr<HacPackWriter> create_file_writer(const std::string &path);

class HacPackBuilder
{
public:
    HacPackBuilder();

    void clear();
    bool empty() const;
    std::size_t file_count() const;

    // Public API: add data buffers
    bool add_entry_from_buffer(const std::string &name,const std::vector<std::uint8_t> &data,std::string &err);
    // Overload: add from raw pointer and size (uint32)
    bool add_entry_from_buffer(const std::string &name,const void *data,std::uint32_t size,std::string &err);

    template<typename T>
    bool add_entry_from_array(const std::string &name,const std::vector<T> &data,std::string &err)
    {
        return add_entry_from_buffer(name, reinterpret_cast<const std::uint8_t*>(data.data()), data.size()*sizeof(T), err);
    }

    // Build the pack and write into provided writer pointer.
    bool build_pack(HacPackWriter *writer,bool index_only,HacPackBuildResult &result,std::string &err) const;

protected:
    bool build_index(std::vector<std::uint8_t> &header,std::vector<std::uint32_t> &offsets,HacPackBuildResult &result,std::string &err) const;

private:
    bool add_entry_internal(std::string name,std::vector<std::uint8_t> data,std::string &err);
    // Overload: internal add using raw pointer and size (uint32)
    bool add_entry_internal(std::string name,const void *data,std::uint32_t size,std::string &err);

    struct Entry
    {
        std::string name;
        std::vector<std::uint8_t> data;
    };

    std::vector<Entry> m_entries;
};

void write_string_list(HacPackBuilder *builder,const std::string &entry_name,const std::vector<std::string> &list,std::string &err);

