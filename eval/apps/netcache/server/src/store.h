#pragma once

#include <arpa/inet.h>
#include <memory>
#include <string>
#include <map>

#include "query.h"
#include "server_reply.h"

#define BUFFER_SIZE 65536

typedef unsigned char byte_t;

namespace netcache {

struct Store {
	std::map<uint8_t, uint32_t> kv_map;

	Store();

	void read_query(const query_t& query);
	void write_query(const query_t& query);
	void del_query(const query_t& query);
};

}  // namespace netcache
