#pragma once

#include "log.h"
#include "util.h"

namespace sycon {

struct buffer_t {
  u8 *data;
  bytes_t size;

  buffer_t() : data(nullptr), size(0) {}

  buffer_t(bytes_t _size) : data(new u8[_size]), size(_size) { std::memset(data, 0, size); }

  buffer_t(u8 *_data, bytes_t _size) : data(new u8[_size]), size(_size) { std::copy(_data, _data + size, data); }

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

    if (data != nullptr) {
      delete[] data;
    }

    size = other.size;
    data = new u8[size];

    std::memset(data, 0, size);
    std::copy(other.data, other.data + other.size, data + (size - other.size));

    return *this;
  }

  buffer_t &operator=(buffer_t &&other) {
    if (this == &other) {
      return *this;
    }

    if (data != nullptr) {
      delete[] data;
    }

    data = other.data;
    size = other.size;

    other.data = nullptr;
    other.size = 0;

    return *this;
  }

  u8 &operator[](bytes_t i) {
    assert(i < size);
    return data[i];
  }

  // Get the value of a buffer at a given offset and width.
  // Returned value is little-endian.
  u64 get(bytes_t offset, bytes_t width) const {
    assert(width > 0 && "Width must be greater than 0");
    assert(offset + width <= size && "Offset and width must be within buffer bounds");
    assert(width <= 8 && "Width must be less than or equal to 8 bytes");

    u64 value = 0;
    for (bytes_t i = 0; i < width; i++) {
      bytes_t index = offset + i;
      assert(index < size);

      value <<= 8;
      value |= data[index];
    }

    return value;
  }

  buffer_t get_slice(bytes_t offset, bytes_t width) const {
    assert(width > 0 && "Width must be greater than 0");
    assert(offset + width <= size && "Offset and width must be within buffer bounds");

    buffer_t slice(width);
    std::copy(data + offset, data + offset + width, slice.data);
    return slice;
  }

  // Set the value of a buffer at a given offset and width.
  // Value is stored in network byte order, i.e., most significant byte first.
  void set(bytes_t offset, bytes_t width, u64 value) {
    assert(width > 0 && "Width must be greater than 0");
    assert(offset + width <= size && "Offset and width must be within buffer bounds");
    assert(width <= 8 && "Width must be less than or equal to 8 bytes");

    for (bytes_t i = 0; i < width; i++) {
      bytes_t index = size - 1 - (offset + i);
      assert(index < size);
      data[index] = value & 0xFF;
      value >>= 8;
    }
  }

  buffer_t reverse() const {
    buffer_t reversed(size);
    for (bytes_t i = 0; i < size; i++) {
      reversed.data[i] = data[size - 1 - i];
    }
    return reversed;
  }

  friend std::ostream &operator<<(std::ostream &os, const buffer_t &buffer);

  std::string to_string() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
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