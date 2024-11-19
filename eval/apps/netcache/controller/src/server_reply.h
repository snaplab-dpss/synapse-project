#pragma once

#include <vector>
#include <stdexcept>
#include <stdint.h>

namespace netcache {

struct server_reply_t {
	bool valid;

	uint16_t key;
	uint32_t val;

	server_reply_t(std::vector<uint8_t>& buffer, ssize_t pkt_size) {
		size_t cur_offset = 0;

		key = read_uint16(buffer, cur_offset);
		cur_offset += sizeof(key);

		val = read_uint32(buffer, cur_offset);
		cur_offset += sizeof(val);
	}

	uint32_t read_uint32(const std::vector<uint8_t>& buffer, size_t offset) const {
		if (offset + sizeof(uint32_t) > buffer.size()) {
			throw std::out_of_range("Buffer overflow during uint32_t read.");
		}

		uint32_t value;
		for (size_t i = 0; i < sizeof(uint32_t); i++) {
			value <<= 8;
			value |= buffer[offset + i];
		}

		return value;
	}

	uint16_t read_uint16(const std::vector<uint8_t>& buffer, size_t offset) const {
		if (offset + sizeof(uint16_t) > buffer.size()) {
			throw std::out_of_range("Buffer overflow during uint16_t read.");
		}

		uint16_t value;
		for (size_t i = 0; i < sizeof(uint16_t); i++) {
			value <<= 8;
			value |= buffer[offset + i];
		}

		return value;
	}

	void serialize_append(std::vector<uint8_t>& buffer, uint64_t value, size_t bytes) const {
		for (auto i = 0; i < bytes; i++) {
			auto byte = (value >> ((bytes - i - 1) * 8)) & 0xff;
			buffer.push_back(byte);
		}
	}

	std::vector<uint8_t> serialize() const {
		auto buffer = std::vector<uint8_t>();

		serialize_append(buffer, key, 2);
		serialize_append(buffer, val, 4);

		return buffer;
	}
};

}  // namespace netcache
