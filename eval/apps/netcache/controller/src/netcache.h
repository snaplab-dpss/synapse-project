#pragma once

#include <algorithm>

#include "packet.h"
#include "tables/fwd.h"
#include "tables/cache_lookup.h"

#include "topology.h"
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
void setup_controller(const topology_t &topology, bool use_tofino_model);
void setup_controller(const std::string &topology_file_path, bool use_tofino_model);

struct bfrt_info_t;

class Controller {
public:
	static std::shared_ptr<Controller> controller;

private:
	const bfrt::BfRtInfo *info;
	std::shared_ptr<bfrt::BfRtSession> session;
	bf_rt_target_t dev_tgt;

	Ports ports;
	topology_t topology;
	bool use_tofino_model;

	// Switch resources
	Fwd fwd;
	CacheLookup cache_lookup;

	Controller(const bfrt::BfRtInfo *_info,
			   		 std::shared_ptr<bfrt::BfRtSession> _session,
			   		 bf_rt_target_t _dev_tgt,
			   		 const topology_t &_topology,
			   		 bool _use_tofino_model)
		: info(_info),
		  session(_session),
		  dev_tgt(_dev_tgt),
		  ports(_info, _session, _dev_tgt),
		  topology(_topology),
		  use_tofino_model(_use_tofino_model),
		  fwd(_info, session, dev_tgt),
		  cache_lookup(_info, session, dev_tgt) { 
		if (!use_tofino_model) {
			config_ports(topology);
		}

		config_stats_port(topology.stats.port, use_tofino_model);

		for (auto connection : topology.connections) {
			auto ig_port = connection.in.port;
			auto eg_port = connection.out.port;

			if (!use_tofino_model) {
				ig_port = ports.get_dev_port(ig_port, 0);
				eg_port = ports.get_dev_port(eg_port, 0);
			}

			// Fwd table entries.
			// fwd.add_entry(ig_port, eg_port);
			
			// Initial cache lookup config.
		}
	}

	void config_stats_port(uint16_t stats_port, bool use_tofino_model);
	void config_ports(const topology_t &topology);

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

	topology_t get_topology() const { return topology; }
	bool get_use_tofino_model() const { return use_tofino_model; }

	static void init(const bfrt::BfRtInfo *_info,
					 std::shared_ptr<bfrt::BfRtSession> _session,
					 bf_rt_target_t _dev_tgt,
					 const topology_t &topology,
					 bool use_tofino_model);
};

}  // namespace netcache
