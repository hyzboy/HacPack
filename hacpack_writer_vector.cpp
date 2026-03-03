#include "hacpack_builder.h"
#include <vector>
#include <memory>

class VectorWriterImpl : public HacPackWriter {
public:
    explicit VectorWriterImpl(std::vector<std::uint8_t> &out) : m_out(out) {}
    bool write(const std::uint8_t *data, std::size_t size, std::string &err) override {
        if (size == 0) return true;
        try {
            m_out.insert(m_out.end(), data, data + size);
        } catch (const std::exception &e) {
            err = e.what();
            return false;
        }
        return true;
    }
private:
    std::vector<std::uint8_t> &m_out;
};

std::unique_ptr<HacPackWriter> create_vector_writer(std::vector<std::uint8_t> &out) {
    return std::unique_ptr<HacPackWriter>(new VectorWriterImpl(out));
}

