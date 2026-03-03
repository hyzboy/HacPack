#include "hacpack_builder.h"
#include <fstream>
#include <memory>
#include <iostream>
#include <string>

class FileWriterImpl : public HacPackWriter {
public:
    explicit FileWriterImpl(const std::string &path)
        : m_out(path, std::ios::binary | std::ios::out | std::ios::trunc), m_path(path)
    {
        if (m_out.is_open() && static_cast<bool>(m_out)) {
            std::cout << "[HacPack][FileWriter] " << m_path << " - Opened output file" << "\n";
        } else {
            std::cerr << "[HacPack][FileWriter] " << m_path << " - Failed to open output file" << "\n";
        }
    }

    ~FileWriterImpl() override {
        if (m_out.is_open()) {
            m_out.flush();
            if (!m_out) {
                std::cerr << "[HacPack][FileWriter] " << m_path << " - Flush failed" << "\n";
            } else {
                std::cout << "[HacPack][FileWriter] " << m_path << " - Flushed file successfully" << "\n";
            }
            m_out.close();
            std::cout << "[HacPack][FileWriter] " << m_path << " - Closed output file" << "\n";
        }
    }

    bool ok() const { return m_out.is_open() && static_cast<bool>(m_out); }

    bool write(const std::uint8_t *data, std::size_t size, std::string &err) override {
        if (size == 0) {
            std::cout << "[HacPack][FileWriter] " << m_path << " - Skipping zero-size write" << "\n";
            return true;
        }
        std::cout << "[HacPack][FileWriter] " << m_path << " - Writing " << size << " bytes" << "\n";
        m_out.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
        if (!m_out) {
            err = "Failed to write to file";
            std::cerr << "[HacPack][FileWriter] " << m_path << " - Write failed (" << size << " bytes)" << "\n";
            return false;
        }
        std::cout << "[HacPack][FileWriter] " << m_path << " - Wrote " << size << " bytes" << "\n";
        return true;
    }

private:
    std::ofstream m_out;
    std::string m_path;
};

std::unique_ptr<HacPackWriter> create_file_writer(const std::string &path) {
    std::cout << "[HacPack][FileWriter] " << path << " - Creating file writer" << "\n";
    auto fw = std::make_unique<FileWriterImpl>(path);
    if (!fw->ok()) {
        std::cerr << "[HacPack][FileWriter] " << path << " - File writer creation failed" << "\n";
        return nullptr;
    }
    std::cout << "[HacPack][FileWriter] " << path << " - File writer created" << "\n";
    return fw;
}

