#pragma once

#include "log.h"
#include "util.h"

#include <cstring>
#include <sstream>
#include <iomanip>

namespace sycon {

struct buffer_t {
  u8 *data;
  bytes_t size;

  explicit buffer_t() : data(nullptr), size(0) {}
  explicit buffer_t(bytes_t _size) : data(new u8[_size]), size(_size) { std::memset(data, 0, size); }
  explicit buffer_t(u8 *_data, bytes_t _size) : data(new u8[_size]), size(_size) { std::copy(_data, _data + size, data); }

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

  u8 get(bytes_t offset) const { return get(offset, 1); }

  buffer_t get_slice(bytes_t offset, bytes_t width) const {
    assert(width > 0 && "Width must be greater than 0");
    assert(offset + width <= size && "Offset and width must be within buffer bounds");

    buffer_t slice(width);
    std::copy(data + offset, data + offset + width, slice.data);
    return slice;
  }

  void set(bytes_t offset, bytes_t width, u64 value) {
    assert(width > 0 && "Width must be greater than 0");
    assert(offset + width <= size && "Offset and width must be within buffer bounds");
    assert(width <= 8 && "Width must be less than or equal to 8 bytes");

    for (bytes_t i = 0; i < width; i++) {
      const bytes_t index = offset + width - 1 - i;
      assert(index < size);
      data[index] = value & 0xFF;
      value >>= 8;
    }
  }

  void set_big_endian(bytes_t offset, bytes_t width, u64 value) {
    assert(width > 0 && "Width must be greater than 0");
    assert(offset + width <= size && "Offset and width must be within buffer bounds");
    assert(width <= 8 && "Width must be less than or equal to 8 bytes");

    for (bytes_t i = 0; i < width; i++) {
      const bytes_t index = offset + i;
      assert(index < size);
      data[index] = value & 0xFF;
      value >>= 8;
    }
  }

  void clear() { std::memset(data, 0, size); }

  buffer_t reverse() const {
    buffer_t reversed(size);
    for (bytes_t i = 0; i < size; i++) {
      reversed.data[i] = data[size - 1 - i];
    }
    return reversed;
  }

  buffer_t shift_right() {
    buffer_t result = *this;
    for (bytes_t i = 0; i < size; i++) {
      const bytes_t rev_i = size - 1 - i;
      result[rev_i] >>= 1;
      if (rev_i > 0) {
        result[rev_i] |= (result[rev_i - 1] & 0x1) << 7;
      }
    }
    return result;
  }

  buffer_t shift_left() {
    buffer_t result = *this;
    for (bytes_t i = 0; i < size; i++) {
      result[i] <<= 1;
      if (i < size - 1) {
        result[i] |= (result[i + 1] >> 7) & 0x1;
      }
    }
    return result;
  }

  buffer_t append(const buffer_t &other) const {
    buffer_t result(size + other.size);
    std::copy(data, data + size, result.data);
    std::copy(other.data, other.data + other.size, result.data + size);
    return result;
  }

  friend std::ostream &operator<<(std::ostream &os, const buffer_t &buffer);

  buffer_t operator^(const buffer_t &other) const {
    const bytes_t this_size  = size;
    const bytes_t other_size = other.size;
    const bytes_t size       = std::max(this_size, other_size);

    buffer_t result(size);
    for (bytes_t i = 0; i < size; i++) {
      u8 this_value  = i < this_size ? data[i] : 0;
      u8 other_value = i < other_size ? other.data[i] : 0;
      result[i]      = this_value ^ other_value;
    }

    return result;
  }

  std::string to_string(bool compact = false) const {
    std::stringstream ss;

    if (compact) {
      for (bytes_t i = 0; i < size; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
      }
    } else {
      ss << *this;
    }

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