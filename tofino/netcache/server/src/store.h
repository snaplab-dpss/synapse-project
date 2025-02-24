#pragma once

#include <arpa/inet.h>
#include <memory>
#include <string>
#include <map>

#include "packet.h"

#define BUFFER_SIZE 65536

typedef unsigned char byte_t;
typedef uint64_t bytes_t;

namespace netcache {

class Store {
private:
  uint16_t port_in;
  uint16_t port_out;
  struct rte_mempool *mbuf_pool;
  struct rte_mbuf *buf[1];
  uint32_t count = 0;

public:
  std::map<uint8_t, uint32_t> kv_map;

  Store(const int in, const int out, rte_mempool *pool);
  ~Store();

  void read_query();
  bool check_pkt(struct rte_mbuf *mbuf);
  void modify_pkt(struct rte_mbuf *mbuf);
};

} // namespace netcache
