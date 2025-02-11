#pragma once

#include <arpa/inet.h>
#include <memory>
#include <string>
#include <map>

#include "packet.h"
#include "query.h"
#include "server_reply.h"

#define BUFFER_SIZE 65536

typedef unsigned char byte_t;

namespace netcache {

class Store {
private:
	uint16_t port_id;
	struct rte_mempool* mbuf_pool;
	struct rte_mbuf *buf[1];

public:
	std::map<uint8_t, uint32_t> kv_map;

	Store(const int dpdk_port, rte_mempool *pool);
	~Store();

	void read_query(const pkt_hdr_t& pkt);
	void test(pkt_hdr_t* pkt);
	// void write_query(const pkt_hdr_t& pkt);
	// void del_query(const pkt_hdr_t& pkt);
};

}  // namespace netcache
