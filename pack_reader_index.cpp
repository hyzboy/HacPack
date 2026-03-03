#include "pack_reader.h"

HacPackIndex::HacPackIndex() = default;

void HacPackIndex::clear() {
    m_entries.clear();
    m_info_size = 0;
    m_data_start = 0;
}

size_t HacPackIndex::file_count() const { return m_entries.size(); }

const std::vector<HacPackEntry>& HacPackIndex::entries() const { return m_entries; }

void HacPackIndex::set_info_size(uint64_t s) { m_info_size = s; }
void HacPackIndex::set_data_start(uint64_t s) { m_data_start = s; }

uint64_t HacPackIndex::info_size() const { return m_info_size; }
uint64_t HacPackIndex::data_start() const { return m_data_start; }

