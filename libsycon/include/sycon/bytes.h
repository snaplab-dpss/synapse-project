#pragma once

#include <assert.h>
#include <stdint.h>

#include <ostream>

namespace sycon {

typedef size_t bit_len_t;
typedef size_t byte_len_t;

typedef uint8_t byte_t;

struct bytes_t {
  byte_t *values;
  byte_len_t size;

  bytes_t() : values(nullptr), size(0) {}

  bytes_t(byte_len_t _size) : values(new byte_t[_size]), size(_size) {}

  bytes_t(byte_len_t _size, uint64_t value) : bytes_t(_size) {
    for (auto i = 0u; i < _size; i++) {
      values[i] = (value >> (8 * (_size - i - 1))) & 0xff;
    }
  }

  bytes_t(bytes_t &&other) : values(other.values), size(other.size) {
    other.values = nullptr;
    other.size = 0;
  }

  bytes_t(const bytes_t &key) : bytes_t(key.size) {
    for (byte_len_t i = 0; i < size; i++) {
      values[i] = key.values[i];
    }
  }

  ~bytes_t() {
    if (values) {
      delete[] values;
      values = nullptr;
      size = 0;
    }
  }

  byte_t &operator[](byte_len_t i) {
    assert(i < size);
    return values[i];
  }

  const byte_t &operator[](byte_len_t i) const {
    assert(i < size);
    return values[i];
  }

  // copy assignment
  bytes_t &operator=(const bytes_t &other) noexcept {
    // Guard self assignment
    if (this == &other) {
      return *this;
    }

    if (size != other.size) {
      this->~bytes_t();
      values = new byte_t[other.size];
      size = other.size;
    }

    std::copy(other.values, other.values + other.size, values);

    return *this;
  }

  struct hash {
    std::size_t operator()(const bytes_t &k) const {
      int hash = 0;

      for (auto i = 0u; i < k.size; i++) {
        hash ^= std::hash<int>()(k.values[i]);
      }

      return hash;
    }
  };
};

inline bool operator==(const bytes_t &lhs, const bytes_t &rhs) {
  if (lhs.size != rhs.size) {
    return false;
  }

  for (auto i = 0u; i < lhs.size; i++) {
    if (lhs.values[i] != rhs.values[i]) {
      return false;
    }
  }

  return true;
}

inline std::ostream &operator<<(std::ostream &os, const bytes_t &bytes) {
  os << "{";
  for (uint32_t i = 0; i < bytes.size; i++) {
    if (i > 0) os << ",";
    os << "0x" << std::hex << (int)bytes[i] << std::dec;
  }
  os << "}";
  return os;
}

}  // namespace sycon