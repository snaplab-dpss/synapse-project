#pragma once

#include <bits/stdint-uintn.h>
#include "../table.h"

namespace netcache {

class IsCpu : public Table {
private:
	struct key_fields_t {
		// Key fields IDs
		bf_rt_id_t ig_port;
	};

	struct data_fields_t {
		// Data field ids
		bf_rt_id_t eg_port;
	};

	struct actions_t {
		// Actions ids
		bf_rt_id_t set_out_port;
	};

	key_fields_t key_fields;
	data_fields_t data_fields;
	actions_t actions;

public:
	IsCpu(const bfrt::BfRtInfo *info,
		std::shared_ptr<bfrt::BfRtSession> session,
		const bf_rt_target_t &dev_tgt)
		: Table(info, session, dev_tgt, "SwitchIngress.is_cpu") {
		init_key({
			{"ig_intr_md.ingress_port", &key_fields.ig_port},
		});

		init_actions({
			{"SwitchIngress.set_out_port", &actions.set_out_port},
		});

		init_data_with_actions({
			{"port", {actions.set_out_port, &data_fields.eg_port}},
		});
	}

	void add_entry(uint32_t ig_port, uint32_t eg_port) {
		key_setup(ig_port);
		data_setup(eg_port, actions.set_out_port);

		auto bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
		assert(bf_status == BF_SUCCESS);
	}

private:
	void key_setup(uint32_t ig_port) {
		table->keyReset(key.get());

		auto bf_status = key->setValue(key_fields.ig_port, static_cast<uint64_t>(ig_port));
		assert(bf_status == BF_SUCCESS);
	}

	void data_setup(uint32_t eg_port, bf_rt_id_t action) {
		auto bf_status = table->dataReset(action, data.get());
		assert(bf_status == BF_SUCCESS);

		bf_status = data->setValue(data_fields.eg_port, static_cast<uint64_t>(eg_port));
		assert(bf_status == BF_SUCCESS);
	}
};

};	// namespace netcache
