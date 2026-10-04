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

  process_report(nc_hdr);

  return false;
}

void Controller::enable_key_expiry() { keys.set_idle_timeout(KEY_IDLE_TIMEOUT_MS, expiry_callback, this); }

void Controller::expiry_callback(const bf_rt_target_t &dev_tgt, const bfrt::BfRtTableKey *key, void *cookie) {
  Controller *controller = reinterpret_cast<Controller *>(cookie);

  std::array<uint8_t, KV_KEY_SIZE> expired_key;
  controller->keys.get_key(key, expired_key.data());

  controller->begin_transaction();

  // The key may have been evicted by a probe between the expiry and this callback.
  if (controller->cached_keys.find(expired_key) != controller->cached_keys.end()) {
    for (const auto &[index, cached_key] : controller->key_storage) {
      if (cached_key != expired_key) {
        continue;
      }

      controller->keys.del_entry(expired_key.data());
      controller->cached_keys.erase(expired_key);
      controller->key_storage[index] = {0};
      controller->available_keys.insert(index);
      controller->reg_v.allocate(index, 0);
      break;
    }
  }

  controller->end_transaction();
}

void Controller::register_digest_callback() {
  bf_status_t bf_status = hh_digest->bfRtLearnCallbackRegister(session, dev_tgt, digest_callback, this);
  ASSERT_BF_STATUS(bf_status);
}

bf_status_t Controller::digest_callback(const bf_rt_target_t &bf_rt_tgt, const std::shared_ptr<bfrt::BfRtSession> session,
                                        std::vector<std::unique_ptr<bfrt::BfRtLearnData>> learn_data, bf_rt_learn_msg_hdl *const learn_msg_hdl,
                                        const void *cookie) {
  Controller *controller = const_cast<Controller *>(reinterpret_cast<const Controller *>(cookie));

  for (const std::unique_ptr<bfrt::BfRtLearnData> &entry : learn_data) {
    // Only the key and the count are read from a report.
    netcache_hdr_t report = {};

    // The key keeps the packet's byte order, which the keys table and the controller's map use.
    bf_status_t bf_status;
    if (controller->hh_digest_key_is_ptr) {
      bf_status = entry->getValue(controller->hh_digest_key_id, KV_KEY_SIZE, report.key);
      ASSERT_BF_STATUS(bf_status);
    } else {
      uint64_t key;
      bf_status = entry->getValue(controller->hh_digest_key_id, &key);
      ASSERT_BF_STATUS(bf_status);
      for (size_t i = 0; i < KV_KEY_SIZE; ++i) {
        report.key[i] = (key >> ((KV_KEY_SIZE - 1 - i) * 8)) & 0xFF;
      }
    }

    uint64_t count;
    bf_status = entry->getValue(controller->hh_digest_count_id, &count);
    ASSERT_BF_STATUS(bf_status);
    for (size_t i = 0; i < KV_VAL_SIZE; ++i) {
      report.val[i] = (count >> (i * 8)) & 0xFF;
    }

    // One transaction per report, as on the CPU-port path: a key is served by the data plane as
    // soon as its own report is handled, not when the whole batch has been.
    controller->begin_transaction();
    controller->process_report(&report);
    controller->end_transaction();
  }

  bf_status_t bf_status = controller->hh_digest->bfRtLearnNotifyAck(session, learn_msg_hdl);
  ASSERT_BF_STATUS(bf_status);

  return BF_SUCCESS;
}

void Controller::process_report(netcache_hdr_t *nc_hdr) {
  DEBUG("It's an HH report (available keys: %lu)", available_keys.size());

  if (!available_keys.empty()) {
    DEBUG("Writing key to cache directly");
    ProcessQuery::process_query->update_cache(nc_hdr);
    return;
  }

  DEBUG("Cache full, probing some keys...");

  // The HH report carries the key's sketch count.
  uint32_t val = 0;
  for (size_t i = 0; i < 4; ++i) {
    val |= (static_cast<uint32_t>(nc_hdr->val[KV_VAL_SIZE - 4 + i]) << (i * 8));
  }

  // Probe random cached keys and evict the first one colder than the reported key: each probe is a
  // hardware register read, so stopping at the first candidate bounds the cost of a report by the
  // probe budget instead of always paying all of it.
  std::uniform_int_distribution<uint16_t> random_index(0, get_cache_capacity() - 1);
  for (uint32_t i = 0; i < args.sample_size; ++i) {
    const uint16_t index        = random_index(random_generator);
    const uint32_t cached_count = reg_key_count.retrieve(index, true);

    if (cached_count >= val) {
      continue;
    }

    const std::array<uint8_t, KV_KEY_SIZE> &key_tmp = key_storage[index];
    uint8_t key[KV_KEY_SIZE];
    std::memcpy(key, key_tmp.data(), sizeof(key_tmp));

    keys.del_entry(key);
    cached_keys.erase(key_tmp);
    key_storage[index] = {0};
    available_keys.insert(index);
    reg_v.allocate(index, 0);
    break;
  }

  ProcessQuery::process_query->update_cache(nc_hdr);
}

} // namespace netcache
