#pragma once

#include <array>
#include <iostream>
#include <limits>
#include <string>

#include "util.h"

namespace sycon {

using field_t = u64;

template <size_t N> struct fields_t {
  std::array<u64, N> values;

  fields_t() {}
  fields_t(const std::array<u64, N> &_values) : values(_values) {}
  fields_t(fields_t<N> &&other) : values(std::move(other.values)) {}
  fields_t(const fields_t<N> &other) : values(other.values) {}

  void operator=(const fields_t &other) { values = other.values; }
  u64 &operator[](size_t i) { return values[i]; }
  const u64 &operator[](size_t i) const { return values[i]; }
};

template <size_t N> inline bool operator==(const fields_t<N> &lhs, const fields_t<N> &rhs) {
  for (size_t i = 0; i < N; i++) {
    if (lhs.values[i] != rhs.values[i]) {
      return false;
    }
  }
  return true;
}

template <size_t N> struct fields_hash_t {
  std::size_t operator()(const fields_t<N> &k) const {
    std::size_t hash = 0;

    for (size_t i = 0; i < N; i++) {
      hash ^= std::hash<u64>{}(k[i]);
    }

    return hash;
  }
};

template <size_t N> using table_key_t   = fields_t<N>;
template <size_t N> using table_value_t = fields_t<N>;

} // namespace sycon