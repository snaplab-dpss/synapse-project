#include "conf.h"

#include <fstream>
#include <iostream>

#include "json.hpp"

namespace netcache {

void from_json(const nlohmann::json &j, misc_t &misc) {
	j.at("interface").get_to(misc.interface);
	j.at("tofino_model").get_to(misc.tofino_model);
	j.at("bf_prompt").get_to(misc.bf_prompt);
}

void from_json(const nlohmann::json &j, kv_t &kv) {
	j.at("store_size").get_to(kv.store_size);
	j.at("initial_entries").get_to(kv.initial_entries);
}

void from_json(const nlohmann::json &j, key_cntr_t &key_cntr) {
	j.at("reg_size").get_to(key_cntr.reg_size);
	j.at("reset_timer").get_to(key_cntr.reset_timer);
}

void from_json(const nlohmann::json &j, cm_t &cm) {
	j.at("reg_size").get_to(cm.reg_size);
	j.at("reset_timer").get_to(cm.reset_timer);
}

void from_json(const nlohmann::json &j, bloom_t &bloom) {
	j.at("reg_size").get_to(bloom.reg_size);
	j.at("reset_timer").get_to(bloom.reset_timer);
}

void from_json(const nlohmann::json &j, topo_port_t &topo_port) {
	j.at("num").get_to(topo_port.num);
	j.at("capacity").get_to(topo_port.capacity);
	j.at("comment").get_to(topo_port.comment);
}

void from_json(const nlohmann::json &j, topo_connection_t &topo_connection) {
	j.at("in").get_to(topo_connection.in);
	j.at("out").get_to(topo_connection.out);
}

void from_json(const nlohmann::json &j, topo_pipes_t &topo_pipes) {
	j.at("external").get_to(topo_pipes.external);
	// j.at("internal").get_to(topo_pipes.internal);
}

void from_json(const nlohmann::json &j, dpdk_t &dpdk) {
	j.at("in").get_to(dpdk.in);
	j.at("out").get_to(dpdk.out);
}

void from_json(const nlohmann::json &j, topology_t &topology) {
	j.at("ports").get_to(topology.ports);
	j.at("pipes").get_to(topology.pipes);
}

void from_json(const nlohmann::json &j, conf_t &conf) {
	j.at("misc").get_to(conf.misc);
	j.at("kv").get_to(conf.kv);
	j.at("key_cntr").get_to(conf.key_cntr);
	j.at("cm").get_to(conf.cm);
	j.at("bloom").get_to(conf.bloom);
	j.at("dpdk").get_to(conf.dpdk);
	j.at("topology").get_to(conf.topology);
}

conf_t parse_conf_file(const std::string &conf_file_path) {
	auto conf_file = std::ifstream(conf_file_path);

	if (!conf_file.good()) {
		std::cerr << "Error opening \"" << conf_file_path << "\". Exiting.\n";
		exit(1);
	}

	auto data = nlohmann::json::parse(conf_file);
	auto conf = data.get<conf_t>();

	std::cout << "\n";
	std::cout << "K/V Store:\n";
	std::cout << "  store size " << conf.kv.store_size << "\n";
	std::cout << "  initial entries (filled) " << conf.kv.initial_entries << "\n";
	std::cout << "CM sketch:\n";
	std::cout << "  reg size " << conf.cm.reg_size << "\n";
	std::cout << "Bloom filter:\n";
	std::cout << "  reg size " << conf.bloom.reg_size << "\n";

	for (auto port : conf.topology.ports) {
		std::cout << "Port:\n";
		std::cout << "	num			" << port.num << "\n";
		std::cout << "	capacity	" << port.capacity << "\n";
		std::cout << "	comment		" << port.comment << "\n";
	}

	return conf;
}
}  // namespace netcache
