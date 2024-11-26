#pragma once

#include <string>

#include "query.h"

namespace netcache {

class Listener {
private:
	int sock_recv;

public:
	Listener(const std::string& iface);

	query_t receive_query();
	~Listener();
};

}  // namespace netcache
