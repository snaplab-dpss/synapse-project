#pragma once

#include <memory>
#include <string>
#include <vector>

#include "packet.h"

#define BUFFER_SIZE 65536

typedef unsigned char byte_t;

namespace netcache {

class ProcessQuery {
public:
	static std::shared_ptr<ProcessQuery> process_query;

	ProcessQuery();
	~ProcessQuery();

	std::vector<std::vector<uint32_t>> sample_values();
	void update_cache(struct netcache_hdr_t* nc_hdr);
};

}  // netcache
