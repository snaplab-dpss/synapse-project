#pragma once

#include <array>
#include <optional>
#include <unordered_set>

#include "../constants.h"
#include "../primitives/table.h"
#include "../time.h"

namespace sycon {

typedef u64 field_t;

template <size_t N>
struct fields_t {
  std::array<u64, N> values;

  fields_t() {}
  fields_t(const std::array<u64, N> &_values) : values(_values) {}
  fields_t(fields_t<N> &&other) : values(std::move(other.values)) {}
  fields_t(const fields_t<N> &other) : values(other.values) {}

  void operator=(const fields_t &other) { values = other.values; }
};

template <size_t N>
bool operator==(const fields_t<N> &lhs, const fields_t<N> &rhs) {
  for (size_t i = 0; i < N; i++) {
    if (lhs.values[i] != rhs.values[i]) {
      return false;
    }
  }
  return true;
}

template <size_t N>
struct fields_hash_t {
  std::size_t operator()(const fields_t<N> &k) const {
    std::size_t hash = 0;

    for (size_t i = 0; i < N; i++) {
      hash ^= std::hash<u64>{}(k.values[i]);
    }

    return hash;
  }
};

template <size_t N>
using key_t = fields_t<N>;

template <size_t N>
using value_t = fields_t<N>;

template <size_t K, size_t V>
class TableMap : public Table {
 private:
  std::unordered_map<key_t<K>, value_t<V>, fields_hash_t<K>> entries;
  bf_rt_id_t populate_action_id;
  std::optional<time_ms_t> timeout;
  std::optional<bfrt::BfRtIdleTmoExpiryCb> callback;

 public:
  TableMap(const std::string &_control_name, const std::string &_table_name)
      : Table(_control_name, _table_name) {
    init_key();
    init_action(SYNAPSE_TABLE_MAP_ACTION, &populate_action_id);
    init_data_with_action(populate_action_id);
  }

  TableMap(const std::string &_control_name, const std::string &_table_name,
           time_ms_t _timeout, const bfrt::BfRtIdleTmoExpiryCb &_callback)
      : TableMap(_control_name, _table_name) {
    timeout = _timeout;
    callback = _callback;
    enable_expirations();
  }

  void enable_expirations() {
    assert(timeout && callback);
    set_notify_mode(*timeout, (void *)this, *callback, true);
  }

  void disable_expirations() {
    assert(timeout && callback);
    set_notify_mode(*timeout, (void *)this, *callback, false);
  }

  // Gets the cached version only.
  bool get(const key_t<K> &k, value_t<V> &v) const {
    // FIXME: the cache may not be up to date, we should query the switch.
    if (entries.find(k) == entries.end()) {
      return false;
    }

    v = entries[k];
    return true;
  }

  void put(const key_t<K> &k, const value_t<V> &v) {
    if (entries.find(k) != entries.end()) {
      // Already put, avoid duplicate insertions.
      return;
    }

    bool found = driver_contains(k);

    if (found) {
      // This entry is definitely on the switch, let's cache it.
      entries[k] = v;
      return;
    }

    driver_add(k, v);
  }

  void del(const key_t<K> &k) {
    if (entries.find(k) != entries.end()) {
      // Avoid duplicate removals.
      entries.erase(k);
    }

    bool found = driver_contains(k);

    if (found) {
      // The entry is still on the switch, the deletion request must have been
      // lost.
      driver_del(k);
    }
  }

  // This is useful for the expiration callback.
  void del(const bfrt::BfRtTableKey *k) { del(build_key(k)); }

 private:
  void key_setup(const key_t<K> &k) {
    auto bf_status = table->keyReset(key.get());
    ASSERT_BF_STATUS(bf_status)

    for (size_t i = 0; i < key_fields.size(); i++) {
      auto key_value = k.values[i];
      auto key_field = key_fields[i];
      auto bf_status = key->setValue(key_field.id, key_value);
      ASSERT_BF_STATUS(bf_status)
    }
  }

  void data_setup(const value_t<V> &v) {
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

  bool driver_contains(const key_t<K> &k) {
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

  bool driver_get(const key_t<K> &k, value_t<V> &v) {
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

  void driver_add(const key_t<K> &k, const value_t<V> &v) {
    log_entry_op("ADD", k, &v);

    key_setup(k);
    data_setup(v);

    u64 flags = 0;
    BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

    auto bf_status =
        table->tableEntryAdd(*session, dev_tgt, flags, *key, *data);

    ASSERT_BF_STATUS(bf_status)
  }

  void driver_mod(const key_t<K> &k, const value_t<V> &v) {
    log_entry_op("MOD", k, &v);

    key_setup(k);
    data_setup(v);

    u64 flags = 0;
    BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

    auto bf_status =
        table->tableEntryMod(*session, dev_tgt, flags, *key, *data);

    ASSERT_BF_STATUS(bf_status)
  }

  void driver_del(const key_t<K> &k) {
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

  key_t<K> build_key(const bfrt::BfRtTableKey *key) const {
    key_t<K> k;

    assert(K == key_fields.size());

    for (size_t i = 0; i < key_fields.size(); i++) {
      auto field = key_fields[i];
      auto bf_status = key->getValue(field.id, &k.values[i]);
      ASSERT_BF_STATUS(bf_status)
    }

    return k;
  }

  value_t<V> build_value(const bfrt::BfRtTableData *value) const {
    value_t<V> v;

    assert(V == data_fields.size());

    for (size_t i = 0; i < data_fields.size(); i++) {
      auto field = data_fields[i];
      auto bf_status = value->getValue(field.id, &v.values[i]);
      ASSERT_BF_STATUS(bf_status)
    }

    return v;
  }

  void log_entry_op(const std::string &op, const key_t<K> &k,
                    const value_t<V> *v = nullptr) const {
    DEBUG();
    DEBUG("*********************************************");

    assert(K > 0);
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
};

}  // namespace sycon