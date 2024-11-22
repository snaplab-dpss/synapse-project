#include "conf.h"

#include <fstream>
#include <iostream>

#include "json.hpp"

namespace netcache {

void from_json(const nlohmann::json &j, kv_t &kv) {
	j.at("store_size").get_to(kv.store_size);
	j.at("initial_entries").get_to(kv.initial_entries);
}

void from_json(const nlohmann::json &j, cm_t &cm) {
	j.at("reg_size").get_to(cm.reg_size);
}

void from_json(const nlohmann::json &j, bloom_t &bloom) {
	j.at("reg_size").get_to(bloom.reg_size);
}

void from_json(const nlohmann::json &j, topo_port_t &topo_port) {
	j.at("port").get_to(topo_port.port);
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

void from_json(const nlohmann::json &j, topology_t &topo_connections) {
	j.at("stats").get_to(topo_connections.stats);
	j.at("connections").get_to(topo_connections.connections);
	j.at("pipes").get_to(topo_connections.pipes);
}

void from_json(const nlohmann::json &j, conf_t &conf) {
	j.at("kv").get_to(conf.kv);
	j.at("cm").get_to(conf.cm);
	j.at("bloom").get_to(conf.bloom);
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
	std::cout << "Stats:\n";
	std::cout << "  port     " << conf.topology.stats.port << "\n";
	std::cout << "  capacity " << conf.topology.stats.capacity << "\n";
	std::cout << "  comment  " << conf.topology.stats.comment << "\n";

	for (auto connection : conf.topology.connections) {
		std::cout << "Connection:\n";
		std::cout << "  [in]  port     " << connection.in.port << "\n";
		std::cout << "  [in]  capacity " << connection.in.capacity << "\n";
		std::cout << "  [in]  comment  " << connection.in.comment << "\n";

		std::cout << "  [out] port     " << connection.out.port << "\n";
		std::cout << "  [out] capacity " << connection.out.capacity << "\n";
		std::cout << "  [out] comment  " << connection.out.comment << "\n";
	}

	return conf;
}
}  // namespace netcache
