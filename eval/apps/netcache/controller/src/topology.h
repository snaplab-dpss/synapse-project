#pragma once

#include <vector>
#include <string>
#include <unistd.h>

namespace netcache {

struct topo_port_t {
	uint16_t port;
	uint32_t capacity;
	std::string comment;
};

struct topo_connection_t {
	topo_port_t in;
	topo_port_t out;
};

struct topo_pipes_t {
	std::vector<uint16_t> external;
	// std::vector<uint16_t> internal;
};

struct topology_t {
	topo_port_t stats;
	std::vector<topo_connection_t> connections;
	topo_pipes_t pipes;
};

topology_t parse_topology_file(const std::string &topology_file_path);

}  // namespace netcache
