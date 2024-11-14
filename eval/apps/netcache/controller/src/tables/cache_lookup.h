#pragma once

#include <bits/stdint-uintn.h>
#include "../table.h"

namespace netcache {

class CacheLookup : public Table {
private:
	struct key_fields_t {
		// Key fields IDs
		bf_rt_id_t nc_key;
	};

	struct data_fields_t {
		// Data field ids
		bf_rt_id_t vt_idx;
		bf_rt_id_t key_idx;
	};

	struct actions_t {
		// Actions ids
		bf_rt_id_t set_lookup_metadata;
	};

	key_fields_t key_fields;
	data_fields_t data_fields;
	actions_t actions;

public:
	CacheLookup(const bfrt::BfRtInfo *info,
				std::shared_ptr<bfrt::BfRtSession> session,
				const bf_rt_target_t &dev_tgt)
				: Table(info, session, dev_tgt, "SwitchIngress.cache_lookup") {
		init_key({
			{"hdr.netcache.key", &key_fields.nc_key},
		});

		init_actions({
			{"SwitchIngress.set_lookup_metadata", &actions.set_lookup_metadata},
		});

		init_data_with_actions({
			{"vt_idx", {actions.set_lookup_metadata, &data_fields.vt_idx}},
			{"key_idx", {actions.set_lookup_metadata, &data_fields.key_idx}},
		});
	}

	void add_entry(uint16_t nc_key, uint16_t vtable_idx, uint16_t key_idx) {
		key_setup(nc_key);
		data_setup(vtable_idx, key_idx, actions.set_lookup_metadata);

		auto bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
		assert(bf_status == BF_SUCCESS);
	}

private:
	void key_setup(uint16_t nc_key) {
		table->keyReset(key.get());

		auto bf_status = key->setValue(key_fields.nc_key, static_cast<uint64_t>(nc_key));
		assert(bf_status == BF_SUCCESS);
	}

	void data_setup(uint16_t vtable_idx, uint16_t key_idx, bf_rt_id_t action) {
		auto bf_status = table->dataReset(action, data.get());
		assert(bf_status == BF_SUCCESS);

		bf_status = data->setValue(data_fields.vt_idx, static_cast<uint64_t>(vtable_idx));
		assert(bf_status == BF_SUCCESS);

		bf_status = data->setValue(data_fields.key_idx, static_cast<uint64_t>(key_idx));
		assert(bf_status == BF_SUCCESS);
	}
};

};	// namespace netcache
