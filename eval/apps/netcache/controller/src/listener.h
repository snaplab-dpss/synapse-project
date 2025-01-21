#pragma once

#include <string>
// #include <rte_mbuf.h>

#include "packet.h"
#include "query.h"

namespace netcache {

class Listener {
private:
	int sock_recv;
	uint8_t* buffer;

	uint16_t port_id;
	// struct rte_mbuf *buf[1];

public:
	Listener(const std::string& iface);

	query_t receive_query();
	~Listener();
};

}  // namespace netcache
