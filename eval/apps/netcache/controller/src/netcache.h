#pragma once

#include <algorithm>
#include <vector>
#include <random>
#include <unordered_set>

#include "constants.h"
#include "packet.h"
#include "tables/fwd.h"
#include "tables/cache_lookup.h"
#include "registers/reg_vtable.h"
#include "registers/reg_key_count.h"
#include "registers/reg_cm_0.h"
#include "registers/reg_cm_1.h"
#include "registers/reg_cm_2.h"
#include "registers/reg_cm_3.h"
#include "registers/reg_bloom_0.h"
#include "registers/reg_bloom_1.h"
#include "registers/reg_bloom_2.h"
#include "conf.h"
#include "ports.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_rt/bf_rt_common.h>
#include <port_mgr/bf_port_if.h>
#ifdef __cplusplus
}
#endif

namespace netcache {

void init_bf_switchd(bool use_tofino_model, bool bf_prompt);
void setup_controller(const conf_t &conf, bool use_tofino_model);
void setup_controller(const std::string &conf_file_path, bool use_tofino_model);

struct bfrt_info_t;

class Controller {
public:
	static std::shared_ptr<Controller> controller;
	conf_t conf;
	// Switch tables
	Fwd fwd;
	CacheLookup cache_lookup;
	// Switch registers
	RegVTable reg_vtable;
	RegKeyCount reg_key_count;
	RegCm0 reg_cm_0;
	RegCm1 reg_cm_1;
	RegCm2 reg_cm_2;
	RegCm3 reg_cm_3;
	RegBloom0 reg_bloom_0;
	RegBloom1 reg_bloom_1;
	RegBloom2 reg_bloom_2;

;private:
	const bfrt::BfRtInfo *info;
	std::shared_ptr<bfrt::BfRtSession> session;
	bf_rt_target_t dev_tgt;

	Ports ports;
	bool use_tofino_model;

	Controller(const bfrt::BfRtInfo *_info,
			   std::shared_ptr<bfrt::BfRtSession> _session,
			   bf_rt_target_t _dev_tgt,
			   const conf_t &_conf,
			   bool _use_tofino_model)
		: info(_info),
		  session(_session),
		  dev_tgt(_dev_tgt),
		  ports(_info, _session, _dev_tgt),
		  conf(_conf),
		  use_tofino_model(_use_tofino_model),
		  fwd(_info, _session, _dev_tgt),
		  cache_lookup(_info, _session, _dev_tgt),
		  reg_vtable(_info, _session, _dev_tgt),
		  reg_key_count(_info, _session, _dev_tgt),
		  reg_cm_0(_info, _session, _dev_tgt),
		  reg_cm_1(_info, _session, _dev_tgt),
		  reg_cm_2(_info, _session, _dev_tgt),
		  reg_cm_3(_info, _session, _dev_tgt),
		  reg_bloom_0(_info, _session, _dev_tgt),
		  reg_bloom_1(_info, _session, _dev_tgt),
		  reg_bloom_2(_info, _session, _dev_tgt) {
		if (!use_tofino_model) {
			config_ports(conf);
		}

		config_stats_port(conf.topology.stats.port, use_tofino_model);

		for (auto connection : conf.topology.connections) {
			auto ig_port = connection.in.port;
			auto eg_port = connection.out.port;

			if (!use_tofino_model) {
				ig_port = ports.get_dev_port(ig_port, 0);
				eg_port = ports.get_dev_port(eg_port, 0);
			}

			#ifndef NDEBUG
			std::cout << "ig_port: " << ig_port << std::endl;
			std::cout << "eg_port: " << eg_port << std::endl;
			#endif

			// Fwd table entries.
			fwd.add_entry(ig_port, READ_QUERY, eg_port);
			fwd.add_entry(ig_port, WRITE_QUERY, eg_port);
			fwd.add_entry(ig_port, DELETE_QUERY, eg_port);
		}

		// Configure mirror session.
		configure_mirroring(128, conf.topology.stats.port);

		// Insert k entries in the switch's KV store, all with value 0.
		// k is defined in conf.kv.initial_entries.

		std::random_device rd;
		std::mt19937 gen(rd());

		std::uniform_int_distribution<> dis(1, conf.kv.store_size);
		std::unordered_set<int> elems;

		while (elems.size() < conf.kv.initial_entries) { elems.insert(dis(gen)); }

		std::vector<int> sampl_index(elems.begin(), elems.end());

		for (int i: sampl_index) { reg_vtable.allocate((uint32_t) i, 0); }

		// Initial cache lookup table config.
		// With this setup, we're assuming that key = key_idx, just to keep it simple.
		cache_lookup.add_entry(0, 0, 0);
		for (int i: sampl_index) {
			cache_lookup.add_entry(i, i, i);
		}
	}

	void config_stats_port(uint16_t stats_port, bool use_tofino_model);
	void config_ports(const conf_t &conf);

public:
	Controller(Controller &other) = delete;
	void operator=(const Controller &) = delete;

	const bfrt::BfRtInfo *get_info() const { return info; }
	std::shared_ptr<bfrt::BfRtSession> get_session() { return session; }
	bf_rt_target_t get_dev_tgt() const { return dev_tgt; }

	struct stats_t {
		uint64_t bytes;
		uint64_t packets;

		stats_t(uint64_t _bytes, uint64_t _packets)
			: bytes(_bytes), packets(_packets) {}
	};

	uint64_t get_port_tx(uint16_t port) { return ports.get_port_tx(port); }

	uint16_t get_dev_port(uint16_t front_panel_port, uint16_t lane) {
		return ports.get_dev_port(front_panel_port, lane);
	}

	conf_t get_conf() const { return conf; }
	bool get_use_tofino_model() const { return use_tofino_model; }

	bf_status_t configure_mirroring(uint16_t session_id_val, uint64_t eg_port) {
		const bfrt::BfRtTable* mirror_cfg = nullptr;

		bf_status_t bf_status = info->bfrtTableFromNameGet("$mirror.cfg", &mirror_cfg);
		assert(bf_status == BF_SUCCESS);

		bf_rt_id_t session_id, normal;
		bf_status = mirror_cfg->keyFieldIdGet("$sid", &session_id);
		assert(bf_status == BF_SUCCESS);
		bf_status = mirror_cfg->actionIdGet("$normal", &normal);
		assert(bf_status == BF_SUCCESS);

		std::unique_ptr<bfrt::BfRtTableKey> mirror_cfg_key;
		std::unique_ptr<bfrt::BfRtTableData> mirror_cfg_data;

		bf_status = mirror_cfg->keyAllocate(&mirror_cfg_key);
		assert(bf_status == BF_SUCCESS);
		bf_status = mirror_cfg->dataAllocate(normal, &mirror_cfg_data);
		assert(bf_status == BF_SUCCESS);

		bf_status = mirror_cfg_key->setValue(session_id, session_id_val);
		assert(bf_status == BF_SUCCESS);

		bf_rt_id_t session_enabled, direction;
		bf_rt_id_t ucast_egress_port, ucast_egress_port_valid, copy_to_cpu;

		bf_status = mirror_cfg->dataFieldIdGet("$session_enable", &session_enabled);
		assert(bf_status == BF_SUCCESS);
		bf_status = mirror_cfg->dataFieldIdGet("$direction", &direction);
		assert(bf_status == BF_SUCCESS);
		bf_status = mirror_cfg->dataFieldIdGet("$ucast_egress_port", &ucast_egress_port);
		assert(bf_status == BF_SUCCESS);
		bf_status = mirror_cfg->dataFieldIdGet("$ucast_egress_port_valid",
											   &ucast_egress_port_valid);
		assert(bf_status == BF_SUCCESS);
		bf_status = mirror_cfg->dataFieldIdGet("$copy_to_cpu", &copy_to_cpu);
		assert(bf_status == BF_SUCCESS);

		bf_status = mirror_cfg_data->setValue(session_enabled, true);
		assert(bf_status == BF_SUCCESS);
		bf_status = mirror_cfg_data->setValue(direction, std::string("EGRESS"));
		assert(bf_status == BF_SUCCESS);
		bf_status = mirror_cfg_data->setValue(ucast_egress_port, eg_port);
		assert(bf_status == BF_SUCCESS);
		bf_status = mirror_cfg_data->setValue(ucast_egress_port_valid, true);
		assert(bf_status == BF_SUCCESS);
		bf_status = mirror_cfg_data->setValue(copy_to_cpu, false);
		assert(bf_status == BF_SUCCESS);

		bf_status = mirror_cfg->tableEntryAdd(*session, dev_tgt, 0,
											  *mirror_cfg_key.get(), *mirror_cfg_data.get());
		assert(bf_status == BF_SUCCESS);

		return BF_SUCCESS;
	}

	static void init(const bfrt::BfRtInfo *_info,
					 std::shared_ptr<bfrt::BfRtSession> _session,
					 bf_rt_target_t _dev_tgt,
					 const conf_t &conf,
					 bool use_tofino_model);
};

}  // namespace netcache
