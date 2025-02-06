#include "netcache.h"
#include "port_stats.h"

namespace netcache {

PortStats::PortStats() {
	conf = netcache::Controller::controller->get_conf();
	use_tofino_model = netcache::Controller::controller->get_use_tofino_model();
}

PortStats::~PortStats() {}

void PortStats::get_stats() {
	// Get tx/rx for all active ports.
	for (auto connection : conf.topology.connections) {
		auto in_port = connection.in.port;

		if (!use_tofino_model) {
			in_port = netcache::Controller::controller->get_dev_port(in_port, 0);
		}

		auto tx = netcache::Controller::controller->get_port_tx(in_port);
		auto rx = netcache::Controller::controller->get_port_rx(in_port);
	}
}

}  // namespace netcache
