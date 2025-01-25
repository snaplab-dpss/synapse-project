#include "conf.h"

#include <fstream>
#include <iostream>

#include "json.hpp"

namespace netcache {

void from_json(const nlohmann::json &j, conf_query_t &query) {
	j.at("limit").get_to(query.limit);
	j.at("duration").get_to(query.duration);
}

void from_json(const nlohmann::json &j, topo_port_t &topo_port) {
	j.at("port").get_to(topo_port.port);
	j.at("comment").get_to(topo_port.comment);
}

void from_json(const nlohmann::json &j, connection_t &topo_connection) {
	j.at("in").get_to(topo_connection.in);
	j.at("out").get_to(topo_connection.out);
}

void from_json(const nlohmann::json &j, conf_t &conf) {
	j.at("query").get_to(conf.query);
	j.at("connections").get_to(conf.connection);
}

conf_t parse_conf_file(const std::string &conf_file_path) {
	auto conf_file = std::ifstream(conf_file_path);

	if (!conf_file.good()) {
		std::cerr << "Error opening \"" << conf_file_path << "\". Exiting.\n";
		exit(1);
	}

	auto data = nlohmann::json::parse(conf_file);
	auto conf = data.get<conf_t>();

	std::cout << "Query:\n";
	std::cout << "  Limit			" << conf.query.limit << "\n";
	std::cout << "  Duration		" << conf.query.duration << "\n";
	std::cout << "Connection:\n";
	std::cout << "  [in]  port		" << conf.connection.in.port << "\n";
	std::cout << "  [in]  comment	" << conf.connection.in.comment << "\n";
	std::cout << "  [out] port		" << conf.connection.out.port << "\n";
	std::cout << "  [out] comment	" << conf.connection.out.comment << "\n";

	return conf;
}
}  // namespace netcache
