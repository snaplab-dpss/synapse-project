#pragma once

#include <array>
#include <iostream>
#include <limits>
#include <string>

#include "log.h"

#define SWAP_ENDIAN_16(v) __bswap_16((v))
#define SWAP_ENDIAN_32(v) __bswap_32((v))

#define WAIT_FOR_ENTER(msg)                                             \
  {                                                                     \
    LOG(msg);                                                           \
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); \
  }

#define SYNAPSE_CONTROLLER_MAIN(argc, argv)                             \
  parse_args(argc, argv);                                               \
  init_switchd();                                                       \
  configure_ports();                                                    \
  register_pcie_pkt_ops();                                              \
  nf_setup();                                                           \
  if (args.run_ucli) {                                                  \
    run_cli();                                                          \
  } else {                                                              \
    WAIT_FOR_ENTER("Controller is running. Press enter to terminate."); \
  }

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

namespace sycon {

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t bit_len_t;
typedef size_t byte_len_t;

typedef u8 byte_t;

typedef u64 field_t;

template <size_t N>
struct fields_t {
  std::array<u64, N> values;

  fields_t() {}
  fields_t(const std::array<u64, N> &_values) : values(_values) {}
  fields_t(fields_t<N> &&other) : values(std::move(other.values)) {}
  fields_t(const fields_t<N> &other) : values(other.values) {}

  void operator=(const fields_t &other) { values = other.values; }
};

template <size_t N>
bool operator==(const fields_t<N> &lhs, const fields_t<N> &rhs) {
  for (size_t i = 0; i < N; i++) {
    if (lhs.values[i] != rhs.values[i]) {
      return false;
    }
  }
  return true;
}

template <size_t N>
struct fields_hash_t {
  std::size_t operator()(const fields_t<N> &k) const {
    std::size_t hash = 0;

    for (size_t i = 0; i < N; i++) {
      hash ^= std::hash<u64>{}(k.values[i]);
    }

    return hash;
  }
};

template <size_t N>
using table_key_t = fields_t<N>;

template <size_t N>
using table_value_t = fields_t<N>;

std::string read_env(const char *env_var);

}  // namespace sycon