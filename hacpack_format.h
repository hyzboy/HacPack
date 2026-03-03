#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace hacpack_format {
inline constexpr char kMagic[4] = {'H', 'A', 'C', '\x1A'};
inline constexpr std::size_t kMagicSize = sizeof(kMagic);
inline constexpr std::uint32_t kVersion = 1;
inline constexpr std::size_t kU32Size = 4;
inline constexpr std::size_t kInfoSizeOffset = kMagicSize;
inline constexpr std::size_t kInfoBlockOffset = kMagicSize + kU32Size;

inline void append_u32_le(std::vector<std::uint8_t> &buf, std::uint32_t v)
{
	for (int i = 0; i < 4; ++i) {
		buf.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
	}
}

inline std::uint32_t read_u32_le_4(const std::uint8_t *b)
{
	return static_cast<std::uint32_t>(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
}

inline bool read_u32_le(const std::vector<std::uint8_t> &buf, std::size_t &pos, std::uint32_t &out)
{
	if (pos + kU32Size > buf.size()) return false;
	out = read_u32_le_4(&buf[pos]);
	pos += kU32Size;
	return true;
}

inline std::uint64_t data_start_offset(std::uint32_t info_size)
{
	return static_cast<std::uint64_t>(kInfoBlockOffset) + info_size;
}
}

