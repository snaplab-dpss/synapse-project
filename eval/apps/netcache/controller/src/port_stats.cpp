#include "netcache.h"
#include "packet.h"
#include "port_stats.h"

namespace netcache {

PortStats::PortStats() {
	conf = netcache::Controller::controller->get_conf();
	use_tofino_model = netcache::Controller::controller->get_use_tofino_model();
}

PortStats::~PortStats() {}

void PortStats::get_stats() {
	std::vector<std::vector<uint64_t>> cur_stats;

	// Get tx/rx for all active ports.
	for (auto port : conf.topology.ports) {
		auto dev_port = port.num;
		if (!use_tofino_model) {
			dev_port = netcache::Controller::controller->get_dev_port(port.num, 0);
		}

		auto tx = netcache::Controller::controller->get_port_tx(dev_port);
		auto rx = netcache::Controller::controller->get_port_rx(dev_port);

		cur_stats.push_back({port.num, rx, tx});
	}

	std::cerr << "STATS";
	for (size_t i = 0; i < cur_stats.size(); ++i) {
		auto cur_port_num = cur_stats[i][0];
		auto cur_rx  = cur_stats[i][1];
		auto cur_tx  = cur_stats[i][2];
		std::cerr << " " << cur_port_num << ":" << cur_rx << ":" << cur_tx;
	}
		std::cerr << std::endl;
}

void PortStats::reset_stats() {
	netcache::Controller::controller->reset_ports();
}

}  // namespace netcache
