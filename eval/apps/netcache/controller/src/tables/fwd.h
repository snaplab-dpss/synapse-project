#pragma once

#include <bits/stdint-uintn.h>
#include "../table.h"

namespace netcache {

class Fwd : public Table {
private:
	struct key_fields_t {
		// Key fields IDs
		bf_rt_id_t eth_dst;
	};

	struct data_fields_t {
		// Data field ids
		bf_rt_id_t port;
	};

	struct actions_t {
		// Actions ids
		bf_rt_id_t set_out_port;
	};

	key_fields_t key_fields;
	data_fields_t data_fields;
	actions_t actions;

public:
	Fwd(const bfrt::BfRtInfo *info,
		std::shared_ptr<bfrt::BfRtSession> session,
		const bf_rt_target_t &dev_tgt)
		: Table(info, session, dev_tgt, "SwitchIngress.fwd") {
		init_key({
			{"hdr.ethernet.dst_addr", &key_fields.eth_dst},
		});

		init_actions({
			{"SwitchIngress.set_out_port", &actions.set_out_port},
		});

		init_data_with_actions({
			{"port", {actions.set_out_port, &data_fields.port}},
		});
	}

	void add_entry(uint64_t eth_dst, uint32_t int_port) {
		key_setup(eth_dst);
		data_setup(int_port, actions.set_out_port);

		auto bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
		assert(bf_status == BF_SUCCESS);
	}

private:
	void key_setup(uint64_t eth_dst) {
		table->keyReset(key.get());

		auto bf_status = key->setValue(key_fields.eth_dst, static_cast<uint64_t>(eth_dst));
		assert(bf_status == BF_SUCCESS);
	}

	void data_setup(uint32_t int_port, bf_rt_id_t action) {
		auto bf_status = table->dataReset(action, data.get());
		assert(bf_status == BF_SUCCESS);

		bf_status = data->setValue(data_fields.port, static_cast<uint64_t>(int_port));
		assert(bf_status == BF_SUCCESS);
	}
};

};	// namespace netcache
