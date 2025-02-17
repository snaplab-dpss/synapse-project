#pragma once

#include "log.h"
#include "util.h"

namespace sycon {

struct buffer_t {
  u8 *data;
  u64 size;

  buffer_t(u64 _size) : size(_size) { data = new u8[size]; }

  buffer_t(const buffer_t &other) : size(other.size) {
    data = new u8[size];
    memcpy(data, other.data, size);
  }

  buffer_t(buffer_t &&other) : data(other.data), size(other.size) {
    other.data = nullptr;
    other.size = 0;
  }

  ~buffer_t() { delete[] data; }

  void operator=(const buffer_t &other) {
    if (&other == this) {
      return;
    }

    if (data != nullptr) {
      delete[] data;
    }

    size = other.size;
    data = new u8[size];
    memcpy(data, other.data, size);
  }

  u8 &operator[](u64 i) { return data[i]; }
};

inline bool operator==(const buffer_t &lhs, const buffer_t &rhs) {
  if (lhs.size != rhs.size) {
    return false;
  }

  for (u64 i = 0; i < lhs.size; i++) {
    if (lhs.data[i] != rhs.data[i]) {
      return false;
    }
  }

  return true;
}

struct buffer_hash_t {
  std::size_t operator()(const buffer_t &buffer) const {
    std::size_t hash = 0;
    for (u64 i = 0; i < buffer.size; i++) {
      hash ^= std::hash<u64>{}(buffer.data[i]);
    }
    return hash;
  }
};

} // namespace sycon