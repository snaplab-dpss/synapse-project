#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <unistd.h>

namespace netcache {

struct misc_t {
	std::string interface;
	bool tofino_model;
	bool bf_prompt;
};

struct kv_t {
	uint16_t store_size;
	uint16_t initial_entries;
};

struct key_cntr_t {
	uint16_t reg_size;
	uint16_t reset_timer;
};

struct cm_t {
	uint16_t reg_size;
	uint16_t reset_timer;
};

struct bloom_t {
	uint16_t reg_size;
	uint16_t reset_timer;
};

struct topo_port_t {
	uint16_t num;
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

struct dpdk_t {
	topo_port_t in;
	topo_port_t out;
};

struct topology_t {
	std::vector<topo_port_t> ports;
	topo_pipes_t pipes;
};

struct conf_t {
	misc_t			misc;
	kv_t			kv;
	key_cntr_t		key_cntr;
	cm_t			cm;
	bloom_t			bloom;
	dpdk_t			dpdk;
	topology_t		topology;
};

conf_t parse_conf_file(const std::string &conf_file_path);

}  // namespace netcache
