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
#include "registers/reg_v0_31.h"
#include "registers/reg_v32_63.h"
#include "registers/reg_v64_95.h"
#include "registers/reg_v96_127.h"
#include "registers/reg_v128_159.h"
#include "registers/reg_v160_191.h"
#include "registers/reg_v192_223.h"
#include "registers/reg_v224_255.h"
#include "registers/reg_v256_287.h"
#include "registers/reg_v288_319.h"
#include "registers/reg_v320_351.h"
#include "registers/reg_v352_383.h"
#include "registers/reg_v384_415.h"
#include "registers/reg_v416_447.h"
#include "registers/reg_v448_479.h"
#include "registers/reg_v480_511.h"
#include "registers/reg_v512_543.h"
#include "registers/reg_v544_575.h"
#include "registers/reg_v576_607.h"
#include "registers/reg_v608_639.h"
#include "registers/reg_v640_671.h"
#include "registers/reg_v672_703.h"
#include "registers/reg_v704_735.h"
#include "registers/reg_v736_767.h"
#include "registers/reg_v768_799.h"
#include "registers/reg_v800_831.h"
#include "registers/reg_v832_863.h"
#include "registers/reg_v864_895.h"
#include "registers/reg_v896_927.h"
#include "registers/reg_v928_959.h"
#include "registers/reg_v960_991.h"
#include "registers/reg_v992_1023.h"
#include "registers/reg_key_count.h"
#include "registers/reg_cm_0.h"
#include "registers/reg_cm_1.h"
#include "registers/reg_cm_2.h"
#include "registers/reg_cm_3.h"
#include "registers/reg_bloom_0.h"
#include "registers/reg_bloom_1.h"
#include "registers/reg_bloom_2.h"
#include "conf.h"
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
void setup_controller(const conf_t &conf, const args_t &args);

struct bfrt_info_t;

class Controller {
public:
  static std::shared_ptr<Controller> controller;

  const bfrt::BfRtInfo *info;
  std::shared_ptr<bfrt::BfRtSession> session;
  bf_rt_target_t dev_tgt;
  Ports ports;

  conf_t conf;
  args_t args;

  // Switch tables
  Fwd fwd;
  Keys keys;
  IsClientPacket is_client_packet;

  // Switch registers
  RegV0_31 reg_v0_31;
  RegV32_63 reg_v32_63;
  RegV64_95 reg_v64_95;
  RegV96_127 reg_v96_127;
  RegV128_159 reg_v128_159;
  RegV160_191 reg_v160_191;
  RegV192_223 reg_v192_223;
  RegV224_255 reg_v224_255;
  RegV256_287 reg_v256_287;
  RegV288_319 reg_v288_319;
  RegV320_351 reg_v320_351;
  RegV352_383 reg_v352_383;
  RegV384_415 reg_v384_415;
  RegV416_447 reg_v416_447;
  RegV448_479 reg_v448_479;
  RegV480_511 reg_v480_511;
  RegV512_543 reg_v512_543;
  RegV544_575 reg_v544_575;
  RegV576_607 reg_v576_607;
  RegV608_639 reg_v608_639;
  RegV640_671 reg_v640_671;
  RegV672_703 reg_v672_703;
  RegV704_735 reg_v704_735;
  RegV736_767 reg_v736_767;
  RegV768_799 reg_v768_799;
  RegV800_831 reg_v800_831;
  RegV832_863 reg_v832_863;
  RegV864_895 reg_v864_895;
  RegV896_927 reg_v896_927;
  RegV928_959 reg_v928_959;
  RegV960_991 reg_v960_991;
  RegV992_1023 reg_v992_1023;
  RegKeyCount reg_key_count;
  RegCm0 reg_cm_0;
  RegCm1 reg_cm_1;
  RegCm2 reg_cm_2;
  RegCm3 reg_cm_3;
  RegBloom0 reg_bloom_0;
  RegBloom1 reg_bloom_1;
  RegBloom2 reg_bloom_2;

  std::map<uint16_t, std::array<uint8_t, 16>> key_storage;
  std::unordered_set<uint16_t> available_keys;

  Controller(const bfrt::BfRtInfo *_info, std::shared_ptr<bfrt::BfRtSession> _session, bf_rt_target_t _dev_tgt, const conf_t &_conf,
             const args_t &_args)
      : info(_info), session(_session), dev_tgt(_dev_tgt), ports(_info, _session, _dev_tgt), conf(_conf), args(_args),
        fwd(_info, _session, _dev_tgt), keys(_info, _session, _dev_tgt),
		is_client_packet(_info, _session, _dev_tgt), reg_v0_31(_info, _session, _dev_tgt),
        reg_v32_63(_info, _session, _dev_tgt), reg_v64_95(_info, _session, _dev_tgt), reg_v96_127(_info, _session, _dev_tgt),
        reg_v128_159(_info, _session, _dev_tgt), reg_v160_191(_info, _session, _dev_tgt), reg_v192_223(_info, _session, _dev_tgt),
        reg_v224_255(_info, _session, _dev_tgt), reg_v256_287(_info, _session, _dev_tgt), reg_v288_319(_info, _session, _dev_tgt),
        reg_v320_351(_info, _session, _dev_tgt), reg_v352_383(_info, _session, _dev_tgt), reg_v384_415(_info, _session, _dev_tgt),
        reg_v416_447(_info, _session, _dev_tgt), reg_v448_479(_info, _session, _dev_tgt), reg_v480_511(_info, _session, _dev_tgt),
        reg_v512_543(_info, _session, _dev_tgt), reg_v544_575(_info, _session, _dev_tgt), reg_v576_607(_info, _session, _dev_tgt),
        reg_v608_639(_info, _session, _dev_tgt), reg_v640_671(_info, _session, _dev_tgt), reg_v672_703(_info, _session, _dev_tgt),
        reg_v704_735(_info, _session, _dev_tgt), reg_v736_767(_info, _session, _dev_tgt), reg_v768_799(_info, _session, _dev_tgt),
        reg_v800_831(_info, _session, _dev_tgt), reg_v832_863(_info, _session, _dev_tgt), reg_v864_895(_info, _session, _dev_tgt),
        reg_v896_927(_info, _session, _dev_tgt), reg_v928_959(_info, _session, _dev_tgt), reg_v960_991(_info, _session, _dev_tgt),
        reg_v992_1023(_info, _session, _dev_tgt), reg_key_count(_info, _session, _dev_tgt), reg_cm_0(_info, _session, _dev_tgt),
        reg_cm_1(_info, _session, _dev_tgt), reg_cm_2(_info, _session, _dev_tgt), reg_cm_3(_info, _session, _dev_tgt),
        reg_bloom_0(_info, _session, _dev_tgt), reg_bloom_1(_info, _session, _dev_tgt), reg_bloom_2(_info, _session, _dev_tgt) {
    if (!args.run_tofino_model) {
      config_ports();
    }

    const uint16_t cpu_port        = args.tna_version == 1 ? CPU_PORT_TNA1 : CPU_PORT_TNA2;
    const uint16_t server_port     = args.server_port;
    const uint16_t server_dev_port = ports.get_dev_port(server_port, 0);

		LOG("CPU port %u", cpu_port);
		LOG("server port %u", server_port);
		LOG("server dev port %u", server_dev_port);

    is_client_packet.add_not_client_port(server_dev_port);
    is_client_packet.add_not_client_port(cpu_port);

    for (uint16_t port : args.client_ports) {
      uint16_t dev_port = port;

      if (!args.run_tofino_model) {
        dev_port = ports.get_dev_port(port, 0);
      }

      LOG("Frontend client port %u -> device port %u", port, dev_port);

      if (port != server_port) {
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

	uint16_t i = 0;
	do {
		key_storage[i] = {0};
		available_keys.insert(i);
	} while (++i != 0);

    // Insert k initial entries in the switch's KV store.
	/*	
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<> dis(0, conf.kv.store_size);
    std::unordered_set<int> elems;

    while (elems.size() < conf.kv.initial_entries) {
      elems.insert(dis(gen));
    }

    std::vector<int> sampl_idx(elems.begin(), elems.end());

    for (int i: sampl_idx) {
		// Pick a value from the set of available keys.
		// Use that value as the index for the k/v to insert in the data plane.
		auto it = available_keys.begin();
		if (i < static_cast<int>(available_keys.size())) {
			std::advance(it, i);
		}

		// Add the index/key to the controller map.
		std::array<uint8_t, 16> key = {};
		key[0] = static_cast<uint8_t>(*it & 0xFF);
		key[1] = static_cast<uint8_t>((*it >> 8) & 0xFF);
		key_storage[*it] = key;

		// Add the index/key to the data plane keys table and reset the key_count[index] register.
		uint8_t key_dp[16];
		std::copy(key.begin(), key.end(), key_dp);
		keys.add_entry(key_dp, *it);
		reg_key_count.allocate(*it, 0);

		// Remove the selected value from the set.
		available_keys.erase(it);
	}
	*/
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

  conf_t get_conf() const { return conf; }
  std::vector<uint16_t> get_client_ports() const { return args.client_ports; }
  bool get_use_tofino_model() const { return args.run_tofino_model; }

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
    bf_status = mirror_cfg_data->setValue(direction, std::string("EGRESS"));
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

  static void init(const bfrt::BfRtInfo *_info, std::shared_ptr<bfrt::BfRtSession> _session, bf_rt_target_t _dev_tgt, const conf_t &conf,
                   const args_t &args);
};

} // namespace netcache
