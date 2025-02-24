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

	/* bool read_query(uint8_t *pkt, uint32_t size); */
	/* struct netcache_hdr_t* check_pkt(struct rte_mbuf *mbuf); */
	std::vector<std::vector<uint32_t>> sample_values();
	void update_cache(struct netcache_hdr_t* nc_hdr);
};

}  // netcache
