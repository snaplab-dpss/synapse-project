#pragma once

#include <string>
#include <memory>
#include <linux/if_packet.h>
#include <sys/socket.h>

#include <rte_mbuf.h>

#include "constants.h"
#include "query.h"

namespace netcache {

class Listener {
private:
	uint8_t* buffer;
	uint32_t count = 0;

	uint16_t port_id;
	struct rte_mbuf *buf[BURST_SIZE];

public:
	Listener(const int dpdk_port);

	pkt_hdr_t receive_query();
	~Listener();
};

}  // namespace netcache
