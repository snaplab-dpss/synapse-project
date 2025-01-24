#pragma once

#include <string>
#include <memory>
#include <linux/if_packet.h>
#include <sys/socket.h>

#include <rte_mbuf.h>

#include "query.h"

namespace netcache {

class Listener {
private:
	uint8_t* buffer;
	uint32_t count;

	/* uint16_t port_id; */
	/* struct rte_mbuf *buf[1]; */

public:
	static std::shared_ptr<Listener> listener_ptr;
	int sockfd;
	struct sockaddr_in serv_addr, ctrl_addr;
	socklen_t ctrl_len = sizeof(ctrl_addr);

	/* Listener(const int dpdk_port); */
	Listener();

	query_t receive_query();
	~Listener();
};

}  // namespace netcache
