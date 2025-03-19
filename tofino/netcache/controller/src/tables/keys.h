#pragma once

#include <bits/stdint-uintn.h>
#include "../table.h"

namespace netcache {

class Keys : public Table {
private:
  struct key_fields_t {
    // Key fields IDs
    bf_rt_id_t cache_key;
  };

  struct data_fields_t {
    // Data field ids
    bf_rt_id_t key_idx;
  };

  struct actions_t {
    // Actions ids
    bf_rt_id_t set_key_idx;
  };

  key_fields_t key_fields;
  data_fields_t data_fields;
  actions_t actions;

public:
  Keys(const bfrt::BfRtInfo *info, std::shared_ptr<bfrt::BfRtSession> session, const bf_rt_target_t &dev_tgt)
      : Table(info, session, dev_tgt, "SwitchIngress.keys") {
    init_key({
        {"hdr.netcache.key", &key_fields.cache_key},
    });

    init_actions({
        {"SwitchIngress.set_key_idx", &actions.set_key_idx},
    });

    init_data_with_actions({
        {"key_idx", {actions.set_key_idx, &data_fields.key_idx}},
    });
  }

  void add_entry(uint8_t *cache_key, uint16_t key_idx) {
    key_setup(cache_key);
    data_setup(key_idx, actions.set_key_idx);

    auto bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
    ASSERT_BF_STATUS(bf_status);
  }

  bool try_add_entry(uint8_t *cache_key, uint16_t key_idx) {
    key_setup(cache_key);
    data_setup(key_idx, actions.set_key_idx);

    auto bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
    return bf_status == BF_SUCCESS;
  }

  void del_entry(uint8_t *cache_key) {
    key_setup(cache_key);

    auto bf_status = table->tableEntryDel(*session, dev_tgt, *key);
    ASSERT_BF_STATUS(bf_status);
  }

private:
  void key_setup(uint8_t *cache_key) {
    table->keyReset(key.get());

    auto bf_status = key->setValue(key_fields.cache_key, cache_key, 16);
    ASSERT_BF_STATUS(bf_status);
  }

  void data_setup(uint16_t key_idx, bf_rt_id_t action) {
    auto bf_status = table->dataReset(action, data.get());
    ASSERT_BF_STATUS(bf_status);

    bf_status = data->setValue(data_fields.key_idx, static_cast<uint64_t>(key_idx));
    ASSERT_BF_STATUS(bf_status);
  }
};

}; // namespace netcache
