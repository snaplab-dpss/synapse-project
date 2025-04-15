#include "netcache.h"
#include "ports.h"
#include "process_query.h"

#include <unistd.h>

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

void Controller::init(const bfrt::BfRtInfo *info, std::shared_ptr<bfrt::BfRtSession> session, bf_rt_target_t dev_tgt, const args_t &args) {
  if (controller) {
    return;
  }

  auto instance = new Controller(info, session, dev_tgt, args);
  controller    = std::shared_ptr<Controller>(instance);
}

bool Controller::process_pkt(pkt_hdr_t *pkt_hdr, uint32_t packet_size) {
  if (!pkt_hdr->has_valid_protocol()) {
    DEBUG("Invalid protocol packet. Ignoring.");
    return false;
  }

  const uint32_t min_size_pkt = pkt_hdr->get_l2_size() + pkt_hdr->get_l3_size() + pkt_hdr->get_l4_size() + pkt_hdr->get_netcache_hdr_size();

  if (packet_size < min_size_pkt) {
    DEBUG("Packet too small. Ignoring.");
    return false;
  }

  // pkt_hdr->pretty_print_base();
  // pkt_hdr->pretty_print_netcache();

  netcache_hdr_t *nc_hdr = pkt_hdr->get_netcache_hdr();

  if (nc_hdr->port == server_dev_port) {
    DEBUG("Server reply, adding entry to cache");
    ProcessQuery::process_query->update_cache(nc_hdr);
    return false;
  }

  DEBUG("It's an HH report packet (available keys: %lu)", available_keys.size());

  if (!available_keys.empty()) {
    if (nc_hdr->op == WRITE_QUERY) {
      DEBUG("Writing key to cache directly");
      ProcessQuery::process_query->update_cache(nc_hdr);
      return false;
    } else {
      DEBUG("Asking server the value entry for this key");
      return true;
    }
  }

  DEBUG("Cache full, probing some keys...");

  std::vector<std::vector<uint32_t>> sampl_vec = ProcessQuery::process_query->sample_values();

  // Retrieve the index of the smallest counter value from the vector.
  uint32_t smallest_val = std::numeric_limits<uint32_t>::max();
  int smallest_idx      = 0;

  for (size_t i = 0; i < sampl_vec.size(); ++i) {
    if (sampl_vec[i][1] < smallest_val) {
      smallest_val = sampl_vec[i][1];
      smallest_idx = i;
    }
  }

  // If the data plane value counter < the HH report counter,
  // Evict the data plane key/value and send the HH report to the server.
  uint32_t val = 0;
  for (size_t i = 0; i < 4; ++i) {
    val |= (static_cast<uint32_t>(nc_hdr->val[KV_VAL_SIZE - 4 + i]) << (i * 8));
  }
  if (sampl_vec[smallest_idx][1] < val) {
    // Remove the key from the keys table and the controller map.
    // Insert the corresponding index to the available_keys set.

    const std::array<uint8_t, KV_KEY_SIZE> &key_tmp = key_storage[sampl_vec[smallest_idx][0]];
    uint8_t key[KV_KEY_SIZE];
    std::memcpy(key, key_tmp.data(), sizeof(key_tmp));

    keys.del_entry(key);
    cached_keys.erase(key_tmp);
    key_storage[sampl_vec[smallest_idx][0]] = {0};
    available_keys.insert(sampl_vec[smallest_idx][0]);

    // Remove the corresponding value from the value registers.

    reg_v0_31.allocate(sampl_vec[smallest_idx][0], 0);
    reg_v32_63.allocate(sampl_vec[smallest_idx][0], 0);
    reg_v64_95.allocate(sampl_vec[smallest_idx][0], 0);
    reg_v96_127.allocate(sampl_vec[smallest_idx][0], 0);
  }

  ProcessQuery::process_query->update_cache(nc_hdr);

  return false;
}

} // namespace netcache
