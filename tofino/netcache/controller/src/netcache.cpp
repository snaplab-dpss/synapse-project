#include "netcache.h"
#include "ports.h"

#include <bf_rt/bf_rt.hpp>

namespace netcache {

std::shared_ptr<Controller> Controller::controller;

void Controller::config_ports() {
  bf_dev_port_t pcie_cpu_port = bf_pcie_cpu_port_get(dev_tgt.dev_id);
  bf_dev_port_t eth_cpu_port  = bf_eth_cpu_port_get(dev_tgt.dev_id);

  bf_port_speed_t pcie_cpu_port_speed;
  uint32_t pcie_cpu_port_lane_number;
  bf_status_t status = bf_port_info_get(dev_tgt.dev_id, pcie_cpu_port, &pcie_cpu_port_speed, &pcie_cpu_port_lane_number);
  if (status != BF_SUCCESS) {
    exit(1);
  }

  std::cout << "PCIe CPU port:	" << pcie_cpu_port << ", " << bf_port_speed_str(pcie_cpu_port_speed);
  std::cout << "ETH CPU port:		" << eth_cpu_port;

  if (!args.run_tofino_model) {
    for (auto port : args.ports) {
      auto dev_port = ports.get_dev_port(port, 0);
      ports.add_dev_port(dev_port, bf_port_speed_t::BF_SPEED_100G);
    }

    if (args.wait_for_ports) {
      for (auto port : args.ports) {
        std::cerr << "Waiting for port " << port << " to be up...\n";
        auto dev_port = ports.get_dev_port(port, 0);
        while (!ports.is_port_up(dev_port)) {
          sleep(1);
        }
      }
    }
  }
}

void Controller::init(const bfrt::BfRtInfo *info, std::shared_ptr<bfrt::BfRtSession> session, bf_rt_target_t dev_tgt, const conf_t &conf,
                      const args_t &args) {
  if (controller) {
    return;
  }

  auto instance = new Controller(info, session, dev_tgt, conf, args);
  controller    = std::shared_ptr<Controller>(instance);
}

} // namespace netcache
