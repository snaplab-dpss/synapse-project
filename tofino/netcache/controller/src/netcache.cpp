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

  LOG("PCIe CPU port: %u (%s)", pcie_cpu_port, bf_port_speed_str(pcie_cpu_port_speed));
  LOG("ETH CPU port: %u", eth_cpu_port);

  if (!args.run_tofino_model) {
    auto server_dev_port = ports.get_dev_port(args.server_port, 0);
    ports.add_dev_port(server_dev_port, bf_port_speed_t::BF_SPEED_100G);

    for (auto port : args.client_ports) {
      auto dev_port = ports.get_dev_port(port, 0);
      ports.add_dev_port(dev_port, bf_port_speed_t::BF_SPEED_100G);
    }

    if (args.wait_for_ports) {
      for (auto port : args.client_ports) {
        LOG("Waiting for port %u to be up...", port);
        auto dev_port = ports.get_dev_port(port, 0);
        while (!ports.is_port_up(dev_port)) {
          sleep(1);
        }
      }

      LOG("Waiting for server port %u to be up...", args.server_port);
      while (!ports.is_port_up(server_dev_port)) {
        sleep(1);
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
