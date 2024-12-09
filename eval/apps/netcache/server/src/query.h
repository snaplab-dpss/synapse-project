#pragma once

#include <netinet/in.h>
#include <vector>
#include <stdexcept>
#include <stdint.h>
#include <iostream>

namespace netcache {

struct query_t {
	bool valid;

	uint8_t op;
	uint32_t seq;
	uint16_t key;
	uint32_t val;

	query_t(std::vector<uint8_t>& buffer, ssize_t pkt_size) {
		valid = true;

		size_t cur_offset = 0;

		op = read_uint8(buffer, cur_offset);
		cur_offset += sizeof(op);

		if (op != 0 && op != 1 && op != 2 && op != 3) {
			valid = false;
		}

		seq = ntohl(read_uint32(buffer, cur_offset));
		cur_offset += sizeof(seq);

		key = ntohs(read_uint16(buffer, cur_offset));
		cur_offset += sizeof(key);

		val = ntohl(read_uint32(buffer, cur_offset));
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

	uint8_t read_uint8(const std::vector<uint8_t>& buffer, size_t offset) const {
		if (offset + sizeof(uint8_t) > buffer.size()) {
			throw std::out_of_range("Buffer overflow during uint8_t read.");
		}

		return buffer[offset];
	}

	void serialize_append(std::vector<uint8_t>& buffer, uint64_t value, size_t bytes) const {
		for (auto i = 0; i < bytes; i++) {
			auto byte = (value >> ((bytes - i - 1) * 8)) & 0xff;
			buffer.push_back(byte);
		}
	}

	std::vector<uint8_t> serialize() const {
		auto buffer = std::vector<uint8_t>();

		serialize_append(buffer, op, 1);
		serialize_append(buffer, seq, 4);
		serialize_append(buffer, key, 2);
		serialize_append(buffer, val, 4);

		return buffer;
	}
};

}  // namespace netcache
