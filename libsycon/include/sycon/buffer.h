#pragma once

#include "log.h"
#include "util.h"

namespace sycon {

struct buffer_t {
  u8 *data;
  bytes_t size;

  buffer_t() : data(nullptr), size(0) {}

  buffer_t(bytes_t _size) : data(new u8[_size]), size(_size) {}

  buffer_t(const buffer_t &other) : buffer_t(other.size) { std::copy(other.data, other.data + size, data); }

  buffer_t(buffer_t &&other) : data(other.data), size(other.size) {
    other.data = nullptr;
    other.size = 0;
  }

  ~buffer_t() {
    if (data != nullptr) {
      delete[] data;
    }

    size = 0;
  }

  buffer_t &operator=(const buffer_t &other) {
    if (this == &other) {
      return *this;
    }

    if (size != other.size) {
      delete[] data;
      size = other.size;
      data = new u8[size];
    }

    std::copy(other.data, other.data + size, data);

    return *this;
  }

  u8 &operator[](bytes_t i) { return data[i]; }

  u64 get(bytes_t offset, bytes_t width) const {
    assert(width > 0 && "Width must be greater than 0");
    assert(offset + width <= size && "Offset and width must be within buffer bounds");
    assert(width <= 8 && "Width must be less than or equal to 8 bytes");

    u64 value = 0;
    for (bytes_t i = 0; i < width; i++) {
      value <<= 8;
      value |= data[offset + i];
    }
    return value;
  }

  void set(bytes_t offset, bytes_t width, u64 value) {
    assert(width > 0 && "Width must be greater than 0");
    assert(offset + width <= size && "Offset and width must be within buffer bounds");
    assert(width <= 8 && "Width must be less than or equal to 8 bytes");

    for (bytes_t i = 0; i < width; i++) {
      data[offset + i] = value & 0xFF;
      value >>= 8;
    }
  }
};

inline bool operator==(const buffer_t &lhs, const buffer_t &rhs) {
  if (lhs.size != rhs.size) {
    return false;
  }

  for (bytes_t i = 0; i < lhs.size; i++) {
    if (lhs.data[i] != rhs.data[i]) {
      return false;
    }
  }

  return true;
}

inline std::ostream &operator<<(std::ostream &os, const buffer_t &buffer) {
  os << "{";
  for (bytes_t i = 0; i < buffer.size; i++) {
    if (i > 0) {
      os << ",";
    }
    os << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)buffer.data[i];
  }
  os << "}";
  os << std::dec;
  return os;
}

struct buffer_hash_t {
  std::size_t operator()(const buffer_t &buffer) const {
    std::size_t hash = 0;
    for (bytes_t i = 0; i < buffer.size; i++) {
      hash ^= std::hash<bytes_t>{}(buffer.data[i]);
    }
    return hash;
  }
};

} // namespace sycon