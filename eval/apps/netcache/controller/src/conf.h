#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <unistd.h>

namespace netcache {

struct kv_t {
	uint16_t store_size;
	uint16_t initial_entries;
};

struct cm_t {
	uint16_t reg_size;
};

struct bloom_t {
	uint16_t reg_size;
};

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

struct conf_t {
	kv_t kv;
	cm_t cm;
	bloom_t bloom;
	topology_t topology;
};

conf_t parse_conf_file(const std::string &conf_file_path);

}  // namespace netcache
