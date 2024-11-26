#pragma once

#include <string>

#include "packet.h"
#include "query.h"

namespace netcache {

class Listener {
private:
	int sock_recv;
	uint8_t* buffer;

public:
	Listener(const std::string& iface);

	query_t receive_query();
	~Listener();
};

}  // namespace netcache
