#pragma once

#include <algorithm>
#include <vector>
#include <random>
#include <unordered_set>
#include <unordered_map>
#include <map>

#include "bf_rt/bf_rt_init.hpp"
#include "constants.h"
#include "packet.h"
#include "tables/fwd.h"
#include "tables/keys.h"
#include "tables/is_client_packet.h"
#include "registers/reg_v.h"
#include "registers/reg_key_count.h"
#include "registers/reg_cm_0.h"
#include "registers/reg_cm_1.h"
#include "registers/reg_cm_2.h"
#include "registers/reg_cm_3.h"
#include "registers/reg_bloom_0.h"
#include "registers/reg_bloom_1.h"
#include "registers/reg_bloom_2.h"
#include "args.h"
#include "ports.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_rt/bf_rt_common.h>
#include <bf_rt/bf_rt_info.h>
#ifdef __cplusplus
}
#endif

namespace netcache {

bf_switchd_context_t *init_bf_switchd(bool bf_prompt, int tna_version);
void run_cli(bf_switchd_context_t *switchd_main_ctx);
void setup_controller(const args_t &args);

struct bfrt_info_t;

// DPDK's implementation of an atomic 16b compare and set operation.
static inline int atomic16_cmpset(volatile uint16_t *dst, uint16_t exp, uint16_t src) {
  uint8_t res;
  asm volatile("lock ; "
               "cmpxchgw %[src], %[dst];"
               "sete %[res];"
               : [res] "=a"(res), /* output */
                 [dst] "=m"(*dst)
               : [src] "r"(src), /* input */
                 "a"(exp), "m"(*dst)
               : "memory"); /* no-clobber list */
  return res;
}

class Controller {
public:
  static std::shared_ptr<Controller> controller;

  const bfrt::BfRtInfo *info;
  std::shared_ptr<bfrt::BfRtSession> session;
  bf_rt_target_t dev_tgt;
  Ports ports;

  args_t args;

  volatile int16_t atom;

  const uint16_t cpu_port;
  const uint16_t server_dev_port;

  // Switch tables
  Fwd fwd;
  Keys keys;
  IsClientPacket is_client_packet;

  // Switch registers
  RegV reg_v;
  RegKeyCount reg_key_count;
  RegCm0 reg_cm_0;
  RegCm1 reg_cm_1;
  RegCm2 reg_cm_2;
  RegCm3 reg_cm_3;
  RegBloom0 reg_bloom_0;
  RegBloom1 reg_bloom_1;
  RegBloom2 reg_bloom_2;

  struct key_hash_t {
    std::size_t operator()(const std::array<uint8_t, KV_KEY_SIZE> &key) const {
      uint32_t hash = 0;
      static_assert(KV_KEY_SIZE == sizeof(uint32_t));
      uint32_t *k = (uint32_t *)key.data();
      hash        = __builtin_ia32_crc32di(hash, *k);
      return hash;
    }
  };

  struct key_cmp_t {
    bool operator()(const std::array<uint8_t, KV_KEY_SIZE> &key1, const std::array<uint8_t, KV_KEY_SIZE> &key2) const {
      return std::memcmp(key1.data(), key2.data(), KV_KEY_SIZE) == 0;
    }
  };

  std::map<uint16_t, std::array<uint8_t, KV_KEY_SIZE>> key_storage;
  std::unordered_set<uint16_t> available_keys;
  std::unordered_set<std::array<uint8_t, KV_KEY_SIZE>, key_hash_t, key_cmp_t> cached_keys;

  Controller(const bfrt::BfRtInfo *_info, std::shared_ptr<bfrt::BfRtSession> _session, bf_rt_target_t _dev_tgt, const args_t &_args)
      : info(_info), session(_session), dev_tgt(_dev_tgt), ports(_info, _session, _dev_tgt), args(_args), atom(0),
        cpu_port(args.tna_version == 1 ? CPU_PORT_TNA1 : CPU_PORT_TNA2), server_dev_port(ports.get_dev_port(args.server_port, 0)),
        fwd(_info, _session, _dev_tgt), keys(_info, _session, _dev_tgt), is_client_packet(_info, _session, _dev_tgt),
        reg_v(_info, _session, _dev_tgt), reg_key_count(_info, _session, _dev_tgt), reg_cm_0(_info, _session, _dev_tgt),
        reg_cm_1(_info, _session, _dev_tgt), reg_cm_2(_info, _session, _dev_tgt), reg_cm_3(_info, _session, _dev_tgt),
        reg_bloom_0(_info, _session, _dev_tgt), reg_bloom_1(_info, _session, _dev_tgt), reg_bloom_2(_info, _session, _dev_tgt) {
    if (!args.run_tofino_model) {
      config_ports();
    }

    LOG("CPU port %u", cpu_port);
    LOG("server port %u", args.server_port);
    LOG("server dev port %u", server_dev_port);

    is_client_packet.add_not_client_port(server_dev_port);
    is_client_packet.add_not_client_port(cpu_port);

    for (uint16_t port : args.client_ports) {
      uint16_t dev_port = port;

      if (!args.run_tofino_model) {
        dev_port = ports.get_dev_port(port, 0);
      }

      LOG("Frontend client port %u -> device port %u", port, dev_port);

      if (port != args.server_port) {
        // Read cache hit.
        fwd.add_entry(dev_port, 1, 0, 0x0, dev_port);
        // Read cache miss (request).
        fwd.add_entry(dev_port, 0, 0, 0x0, server_dev_port);
        // Read cache miss (reply).
        fwd.add_entry(server_dev_port, 0, dev_port, 0xFFFF, dev_port);
        fwd.add_entry(server_dev_port, 1, dev_port, 0xFFFF, dev_port);
      }
    }

    // HH report (request).
    fwd.add_entry(cpu_port, 0, 0, 0x0, server_dev_port);
    fwd.add_entry(cpu_port, 1, 0, 0x0, server_dev_port);

    // HH report (reply).
    fwd.add_entry(server_dev_port, 0, cpu_port, 0xFFFF, cpu_port);
    fwd.add_entry(server_dev_port, 1, cpu_port, 0xFFFF, cpu_port);

    // Configure mirror session.
    configure_mirroring(128, cpu_port);

    if (!args.cache_activated) {
      return;
    }

    // Initialize the key storage map and available keys set.
    for (uint64_t i = 0; i < get_cache_capacity(); ++i) {
      key_storage[i] = {0};
      available_keys.insert(i);
    }
  }

public:
  Controller(Controller &other)      = delete;
  void operator=(const Controller &) = delete;

  const bfrt::BfRtInfo *get_info() const { return info; }
  std::shared_ptr<bfrt::BfRtSession> get_session() { return session; }
  bf_rt_target_t get_dev_tgt() const { return dev_tgt; }

  void config_ports();

  uint64_t get_port_tx(uint16_t port) { return ports.get_port_tx(port); }
  uint64_t get_port_rx(uint16_t port) { return ports.get_port_rx(port); }

  void reset_ports() { ports.reset_ports(); }

  uint16_t get_dev_port(uint16_t front_panel_port, uint16_t lane) { return ports.get_dev_port(front_panel_port, lane); }

  std::vector<uint16_t> get_client_ports() const { return args.client_ports; }
  bool get_use_tofino_model() const { return args.run_tofino_model; }

  size_t get_cache_capacity() const {
    size_t capacity = keys.get_size();

    if (capacity > 1024) {
      // Actually, we usually only get 90% of usage from the dataplane tables.
      // Higher than that and we start getting collisions, and errors trying to insert new entries.
      capacity *= 0.9;
    }
    return capacity;
  }

  bf_status_t configure_mirroring(uint16_t session_id_val, uint64_t eg_port) {
    const bfrt::BfRtTable *mirror_cfg = nullptr;

    bf_status_t bf_status = info->bfrtTableFromNameGet("$mirror.cfg", &mirror_cfg);
    ASSERT_BF_STATUS(bf_status);

    bf_rt_id_t session_id, normal;
    bf_status = mirror_cfg->keyFieldIdGet("$sid", &session_id);
    ASSERT_BF_STATUS(bf_status);
    bf_status = mirror_cfg->actionIdGet("$normal", &normal);
    ASSERT_BF_STATUS(bf_status);

    std::unique_ptr<bfrt::BfRtTableKey> mirror_cfg_key;
    std::unique_ptr<bfrt::BfRtTableData> mirror_cfg_data;

    bf_status = mirror_cfg->keyAllocate(&mirror_cfg_key);
    ASSERT_BF_STATUS(bf_status);
    bf_status = mirror_cfg->dataAllocate(normal, &mirror_cfg_data);
    ASSERT_BF_STATUS(bf_status);

    bf_status = mirror_cfg_key->setValue(session_id, session_id_val);
    ASSERT_BF_STATUS(bf_status);

    bf_rt_id_t session_enabled, direction;
    bf_rt_id_t ucast_egress_port, ucast_egress_port_valid, copy_to_cpu;

    bf_status = mirror_cfg->dataFieldIdGet("$session_enable", &session_enabled);
    ASSERT_BF_STATUS(bf_status);
    bf_status = mirror_cfg->dataFieldIdGet("$direction", &direction);
    ASSERT_BF_STATUS(bf_status);
    bf_status = mirror_cfg->dataFieldIdGet("$ucast_egress_port", &ucast_egress_port);
    ASSERT_BF_STATUS(bf_status);
    bf_status = mirror_cfg->dataFieldIdGet("$ucast_egress_port_valid", &ucast_egress_port_valid);
    ASSERT_BF_STATUS(bf_status);
    bf_status = mirror_cfg->dataFieldIdGet("$copy_to_cpu", &copy_to_cpu);
    ASSERT_BF_STATUS(bf_status);

    bf_status = mirror_cfg_data->setValue(session_enabled, true);
    ASSERT_BF_STATUS(bf_status);
    bf_status = mirror_cfg_data->setValue(direction, std::string("INGRESS"));
    ASSERT_BF_STATUS(bf_status);
    bf_status = mirror_cfg_data->setValue(ucast_egress_port, eg_port);
    ASSERT_BF_STATUS(bf_status);
    bf_status = mirror_cfg_data->setValue(ucast_egress_port_valid, true);
    ASSERT_BF_STATUS(bf_status);
    bf_status = mirror_cfg_data->setValue(copy_to_cpu, false);
    ASSERT_BF_STATUS(bf_status);

    bf_status = mirror_cfg->tableEntryAdd(*session, dev_tgt, 0, *mirror_cfg_key.get(), *mirror_cfg_data.get());
    ASSERT_BF_STATUS(bf_status);

    return BF_SUCCESS;
  }

  void lock() {
    while (!atomic16_cmpset((volatile uint16_t *)&atom, 0, 1)) {
      // prevent the compiler from removing this loop
      __asm__ __volatile__("");
    }
  }

  void unlock() { atom = 0; }

  void begin_transaction() {
    lock();
    const bool atomic     = true;
    bf_status_t bf_status = session->beginTransaction(atomic);
    ASSERT_BF_STATUS(bf_status);
  }

  void end_transaction() {
    const bool block_until_complete = true;
    bf_status_t bf_status           = session->commitTransaction(block_until_complete);

    ASSERT_BF_STATUS(bf_status);
    unlock();
  }

  void begin_batch() {
    lock();
    bf_status_t bf_status = session->beginBatch();
    ASSERT_BF_STATUS(bf_status);
  }

  void end_batch() {
    bf_status_t bf_status = session->endBatch(true);
    ASSERT_BF_STATUS(bf_status);
    unlock();
  }

  bool process_pkt(pkt_hdr_t *pkt_hdr, uint32_t packet_size);

  static void init(const bfrt::BfRtInfo *_info, std::shared_ptr<bfrt::BfRtSession> _session, bf_rt_target_t _dev_tgt, const args_t &args);
};

} // namespace netcache
