#include "netcache.h"
#include "ports.h"

#include <bf_rt/bf_rt.hpp>

namespace netcache {

std::shared_ptr<Controller> Controller::controller;

void Controller::config_ports(const topology_t &topology) {
	auto stats_speed = Ports::gbps_to_bf_port_speed(topology.stats.capacity);
	ports.add_port(topology.stats.port, 0, stats_speed);

	for (auto connection : topology.connections) {
		auto in_speed = Ports::gbps_to_bf_port_speed(connection.in.capacity);
		auto out_speed = Ports::gbps_to_bf_port_speed(connection.out.capacity);

		auto in_dev_port = ports.get_dev_port(connection.in.port, 0);
		auto out_dev_port = ports.get_dev_port(connection.out.port, 0);

		ports.add_dev_port(in_dev_port, in_speed);
		ports.add_dev_port(out_dev_port, out_speed);

		std::cerr << "Waiting for ports " << connection.in.port << " and "
				  << connection.out.port << " to be up...\n";

		while (!ports.is_port_up(in_dev_port) ||
			   !ports.is_port_up(out_dev_port)) {
			sleep(1);
		}
	}
}

void Controller::config_stats_port(uint16_t stats_port, bool use_tofino_model) {
	auto front_panel = stats_port;

	if (!use_tofino_model) {
		stats_port = ports.get_dev_port(stats_port, 0);
	}

	p4_devport_mgr_set_copy_to_cpu(0, true, stats_port);

	if (!use_tofino_model) {
		std::cerr << "Waiting for stats port " << front_panel << " to be up...\n";

		while (!ports.is_port_up(stats_port)) {
			sleep(1);
		}
	}
}

void Controller::init(const bfrt::BfRtInfo *info,
					  std::shared_ptr<bfrt::BfRtSession> session,
					  bf_rt_target_t dev_tgt,
					  const topology_t &topology,
					  bool use_tofino_model) {
	if (controller) {
		return;
	}

	auto instance = new Controller(info, session, dev_tgt, topology, use_tofino_model);
	controller = std::shared_ptr<Controller>(instance);
}

}  // namespace netcache
