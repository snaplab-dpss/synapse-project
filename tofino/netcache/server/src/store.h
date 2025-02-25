#pragma once

#include <arpa/inet.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <array>
#include <cstring>

#include "packet.h"
#include "constants.h"

#define BUFFER_SIZE 65536

typedef unsigned char byte_t;
typedef uint64_t bytes_t;

namespace netcache {

struct hash_key_128 {
    std::size_t operator()(const std::array<uint8_t, 16>& key) const {
        std::size_t hash = 0;
        for (int i = 0; i < 16; ++i) {
            hash ^= std::hash<uint8_t>()(key[i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

struct hash_key_128_equal {
    bool operator()(const std::array<uint8_t, 16>& key1,
					const std::array<uint8_t, 16>& key2) const {
        return std::memcmp(key1.data(), key2.data(), 16) == 0;
    }
};

class Store {
private:
  uint16_t port_in;
  uint16_t port_out;
  struct rte_mempool *mbuf_pool;
  struct rte_mbuf *buf[1];
  uint32_t count = 0;

public:
  std::unordered_map<std::array<uint8_t, 16>, uint8_t[], hash_key_128, hash_key_128_equal> kv_map;

  Store(const int in, const int out, rte_mempool *pool);
  ~Store();

  void read_query();
  bool check_pkt(struct rte_mbuf *mbuf);
  void modify_pkt(struct rte_mbuf *mbuf);
};

} // namespace netcache
