#pragma once

#include <memory>
#include <string>
#include <rte_mbuf.h>
#include <rte_ethdev.h>
#include <rte_malloc.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_tcp.h>
#include <vector>

#include "packet.h"

#define BUFFER_SIZE 65536

typedef unsigned char byte_t;

namespace netcache {

class ProcessQuery {
private:
	uint16_t port_in;
	uint16_t port_out;
	struct rte_mempool* mbuf_pool;
	struct rte_mbuf *buf[1];

public:
	ProcessQuery(const int in, const int out, rte_mempool *pool);
	~ProcessQuery();

	void read_query();
	struct netcache_hdr_t* check_pkt(struct rte_mbuf *mbuf);
	std::vector<std::vector<uint32_t>> sample_values();
	void update_cache(struct rte_mbuf **buf,
					  std::vector<std::vector<uint32_t>> sampl_cntr,
					  int smallest_idx,
					  struct netcache_hdr_t* nc_hdr);
};

}  // netcache
