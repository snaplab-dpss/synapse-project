#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <unistd.h>

namespace netcache {

struct conf_query_t {
	uint32_t limit;
	uint32_t wait;
};

struct topo_port_t {
	uint16_t port;
	std::string comment;
};

struct connection_t {
	topo_port_t in;
	topo_port_t out;
};

struct conf_t {
	conf_query_t	query;
	connection_t	connection;
};

conf_t parse_conf_file(const std::string &conf_file_path);

}  // namespace netcache
