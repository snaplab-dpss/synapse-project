#include "netcache.h"
#include "ports.h"

#include <bf_rt/bf_rt.hpp>

namespace netcache {

std::shared_ptr<Controller> Controller::controller;

void Controller::config_ports(const conf_t &conf) {
	for (auto port : conf.topology.ports) {
		auto speed = Ports::gbps_to_bf_port_speed(port.capacity);
		auto dev_port = ports.get_dev_port(port.num, 0);

		ports.add_dev_port(dev_port, speed);

		std::cerr << "Waiting for port " << port.num << " to be up...\n";
		while (!ports.is_port_up(dev_port)) {
			sleep(1);
		}
	 }
}

void Controller::init(const bfrt::BfRtInfo *info,
					  std::shared_ptr<bfrt::BfRtSession> session,
					  bf_rt_target_t dev_tgt,
					  const conf_t &conf,
					  bool use_tofino_model) {
	if (controller) {
		return;
	}

	auto instance = new Controller(info, session, dev_tgt, conf, use_tofino_model);
	controller = std::shared_ptr<Controller>(instance);
}

}  // namespace netcache
