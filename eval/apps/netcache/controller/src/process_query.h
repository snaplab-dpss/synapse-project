#pragma once

#include <memory>
#include <string>

#include "packet.h"
#include "query.h"
#include "server_reply.h"

#define BUFFER_SIZE 65536

typedef unsigned char byte_t;

namespace netcache {

class ProcessQuery {
private:
	uint16_t port_id;
	// struct rte_mempool* mbuf_pool;
	// struct rte_mbuf *buf[1];

public:
	ProcessQuery(const int dpdk_port);
	~ProcessQuery();

	// void read_query(const pkt_hdr_t& pkt);

};

}  // netcache
