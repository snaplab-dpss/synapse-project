#pragma once

#include <array>
#include <optional>
#include <unordered_set>

#include "../constants.h"
#include "../primitives/table.h"
#include "../time.h"
#include "../util.h"

namespace sycon {

template <size_t K, size_t V>
class TableMap : public Table {
  static_assert(K > 0);
  static_assert(V > 0);

 private:
  std::unordered_map<table_key_t<K>, table_value_t<V>, fields_hash_t<K>> cache;

  bf_rt_id_t populate_action_id;
  std::optional<time_ms_t> timeout;

 public:
  TableMap(const std::string &_control_name, const std::string &_table_name)
      : Table(_control_name, _table_name) {
    init_key();
    init_action(SYNAPSE_TABLE_MAP_ACTION, &populate_action_id);
    init_data_with_action(populate_action_id);
  }

  TableMap(const std::string &_control_name, const std::string &_table_name,
           time_ms_t _timeout)
      : TableMap(_control_name, _table_name) {
    timeout = _timeout;
    enable_expirations();
  }

  void enable_expirations() {
    assert(timeout);
    set_notify_mode(*timeout, (void *)this, internal_expiration_callback, true);
  }

  void disable_expirations() {
    assert(timeout);
    set_notify_mode(*timeout, (void *)this, internal_expiration_callback,
                    false);
  }

  // Gets the cached version only.
  bool get(const table_key_t<K> &k, table_value_t<V> &v) const {
    if (cache.find(k) == cache.end()) {
      return false;
    }

    v = cache[k];
    return true;
  }

  void put(const table_key_t<K> &k, const table_value_t<V> &v) {
    // This entry is already on the switch.
    if (cache.find(k) != cache.end()) {
      return;
    }

    driver_add(k, v);
    cache[k] = v;
  }

  void del(const table_key_t<K> &k) {
    if (cache.find(k) == cache.end()) {
      // Avoid duplicate removals.
      return;
    }

    driver_del(k);
    cache.erase(k);
  }

 private:
  void key_setup(const table_key_t<K> &k) {
    auto bf_status = table->keyReset(key.get());
    ASSERT_BF_STATUS(bf_status)

    for (size_t i = 0; i < key_fields.size(); i++) {
      auto key_value = k.values[i];
      auto key_field = key_fields[i];
      auto bf_status = key->setValue(key_field.id, key_value);
      ASSERT_BF_STATUS(bf_status)
    }
  }

  void data_setup(const table_value_t<V> &v) {
    auto bf_status = table->dataReset(populate_action_id, data.get());
    ASSERT_BF_STATUS(bf_status)

    assert(V == data_fields.size());

    for (size_t i = 0; i < V; i++) {
      auto param_field_value = v.values[i];
      auto param_field = data_fields[i];
      bf_status = data->setValue(param_field.id, param_field_value);
      ASSERT_BF_STATUS(bf_status)
    }

    if (time_aware) {
      bf_status = data->setValue(entry_ttl_data_id, static_cast<u64>(*timeout));
      ASSERT_BF_STATUS(bf_status)
    }
  }

  bool driver_contains(const table_key_t<K> &k) {
    key_setup(k);

    u64 flags = 0;
    BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

    bf_rt_handle_t entry_handle;
    bf_status_t bf_status = table->tableEntryHandleGet(*session, dev_tgt, flags,
                                                       *key, &entry_handle);

    if (bf_status == BF_OBJECT_NOT_FOUND) {
      return false;
    }

    ASSERT_BF_STATUS(bf_status)
    return true;
  }

  bool driver_get(const table_key_t<K> &k, table_value_t<V> &v) {
    key_setup(k);

    u64 flags = 0;
    BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

    bf_rt_handle_t entry_handle;
    bf_status_t bf_status = table->tableEntryHandleGet(*session, dev_tgt, flags,
                                                       *key, &entry_handle);

    if (bf_status == BF_OBJECT_NOT_FOUND) {
      return false;
    }

    ASSERT_BF_STATUS(bf_status)

    bfrt::BfRtTableKey key;
    bfrt::BfRtTableData value;

    bf_status = table->tableEntryGet(*session, dev_tgt, flags, entry_handle,
                                     &key, &value);

    if (bf_status == BF_OBJECT_NOT_FOUND) {
      return false;
    }

    ASSERT_BF_STATUS(bf_status)

    v = build_value(value);
    return true;
  }

  void driver_add(const table_key_t<K> &k, const table_value_t<V> &v) {
    log_entry_op("ADD", k, &v);

    key_setup(k);
    data_setup(v);

    u64 flags = 0;
    BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

    auto bf_status =
        table->tableEntryAdd(*session, dev_tgt, flags, *key, *data);

    ASSERT_BF_STATUS(bf_status)
  }

  void driver_mod(const table_key_t<K> &k, const table_value_t<V> &v) {
    log_entry_op("MOD", k, &v);

    key_setup(k);
    data_setup(v);

    u64 flags = 0;
    BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

    auto bf_status =
        table->tableEntryMod(*session, dev_tgt, flags, *key, *data);

    ASSERT_BF_STATUS(bf_status)
  }

  void driver_del(const table_key_t<K> &k) {
    log_entry_op("DEL", k);

    key_setup(k);

    u64 flags = 0;
    BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

    auto bf_status = table->tableEntryDel(*session, dev_tgt, flags, *key);
    ASSERT_BF_STATUS(bf_status)
  }

  void driver_clear() {
    u64 flags = 0;
    BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

    auto bf_status = table->tableClear(*session, dev_tgt, flags);
    ASSERT_BF_STATUS(bf_status)

    DEBUG();
    DEBUG("*********************************************");
    DEBUG("Time: %lu\n", time(0));
    DEBUG("[%s] Clear\n", name.c_str());
    DEBUG("*********************************************");
  }

  table_key_t<K> build_key(const bfrt::BfRtTableKey *key) const {
    table_key_t<K> k;

    assert(K == key_fields.size());

    for (size_t i = 0; i < key_fields.size(); i++) {
      auto field = key_fields[i];
      auto bf_status = key->getValue(field.id, &k.values[i]);
      ASSERT_BF_STATUS(bf_status)
    }

    return k;
  }

  table_value_t<V> build_value(const bfrt::BfRtTableData *value) const {
    table_value_t<V> v;

    assert(V == data_fields.size());

    for (size_t i = 0; i < data_fields.size(); i++) {
      auto field = data_fields[i];
      auto bf_status = value->getValue(field.id, &v.values[i]);
      ASSERT_BF_STATUS(bf_status)
    }

    return v;
  }

  void log_entry_op(const std::string &op, const table_key_t<K> &k,
                    const table_value_t<V> *v = nullptr) const {
    DEBUG();
    DEBUG("*********************************************");

    assert(K == key_fields.size());

    DEBUG("Time: %lu", time(0));
    DEBUG("[%s] Op: %s", name.c_str(), op.c_str());

    DEBUG("  Key:");
    for (size_t i = 0; i < K; i++) {
      const table_field_t &key_field = key_fields[i];
      const u64 &key_value = k.values[i];
      DEBUG("     %s : 0x%02lx", key_field.name.c_str(), key_value);
    }

    if (v) {
      assert(V == data_fields.size());

      DEBUG("  Data:");
      for (size_t i = 0; i < V; i++) {
        const table_field_t &data_field = data_fields[i];
        const u64 &data_value = v->values[i];
        DEBUG("     %s : 0x%02lx", data_field.name.c_str(), data_value);
      }

      DEBUG("*********************************************\n");
    }
  }

  static bf_status_t internal_expiration_callback(const bf_rt_target_t &target,
                                                  const bfrt::BfRtTableKey *key,
                                                  void *cookie) {
    TableMap *tm = static_cast<TableMap *>(cookie);
    table_key_t<K> k = tm->build_key(key);

    cfg.begin_transaction();
    tm->del(k);
    cfg.end_transaction();

    return BF_SUCCESS;
  }
};

}  // namespace sycon