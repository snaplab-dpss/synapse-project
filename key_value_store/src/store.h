#pragma once

#include <arpa/inet.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <array>
#include <cstring>

#include "packet.h"
#include "constants.h"

typedef unsigned char byte_t;
typedef uint64_t bytes_t;

namespace netcache {

using key_t   = std::array<uint8_t, KV_KEY_SIZE>;
using value_t = std::array<uint8_t, KV_VAL_SIZE>;

struct key_hash_t {
  std::size_t operator()(const key_t &key) const {
    unsigned long long hash = 0;
    unsigned long long *k   = (unsigned long long *)key.data();
    hash                    = __builtin_ia32_crc32di(hash, *k);
    hash                    = __builtin_ia32_crc32di(hash, *(k + 1));
    return hash;
  }
};

struct key_cmp_t {
  bool operator()(const key_t &key1, const key_t &key2) const { return std::memcmp(key1.data(), key2.data(), KV_KEY_SIZE) == 0; }
};

class Store {
private:
  const int64_t processing_delay_ns;
  const uint16_t port_in;
  const uint16_t port_out;
  const uint16_t queue;

public:
  std::unordered_map<key_t, value_t, key_hash_t, key_cmp_t> kv_map;

  Store(const int64_t processing_delay_ns, const int in, const int out, const uint16_t rx_queue);
  ~Store();

  void run();

private:
  bool check_pkt(struct rte_mbuf *mbuf);
  void process_netcache_query(struct rte_mbuf *mbuf);
};

} // namespace netcache
