#pragma once

#include <array>
#include <optional>
#include <unordered_set>

#include "../primitives/table.h"
#include "../primitives/register.h"
#include "../primitives/digest.h"
#include "../time.h"
#include "../field.h"
#include "../hash.h"
#include "synapse_ds.h"

namespace sycon {

class FCFSCachedTable : public SynapseDS {
private:
  std::unordered_map<buffer_t, u32, buffer_hash_t> cache;

  std::vector<Table> tables;
  Register reg_alarm;
  Digest digest;

  const u32 capacity;
  const u32 cache_capacity;
  const bytes_t key_size;
  const bits_t cache_hash_size;

public:
  FCFSCachedTable(const std::string &_name, const std::vector<std::string> &table_names, const std::string &reg_alarm_name,
                  const std::string &digest_name, std::optional<time_ms_t> timeout = std::nullopt)
      : SynapseDS(_name), tables(build_tables(table_names)), reg_alarm(reg_alarm_name), digest(digest_name), capacity(get_capacity(tables)),
        cache_capacity(reg_alarm.get_capacity()), key_size(get_key_size(tables)), cache_hash_size(get_cache_hash_size(cache_capacity)) {
    if (timeout.has_value()) {
      Table &chosen_expiration_table = tables.front();
      chosen_expiration_table.set_notify_mode(timeout.value(), this, FCFSCachedTable::expiration_callback, true);
    }

    digest.register_callback(FCFSCachedTable::digest_callback, this);
  }

  bool get(const buffer_t &k, u32 &v) const {
    auto found_it = cache.find(k);
    if (found_it == cache.end()) {
      return false;
    }

    v = found_it->second;
    return true;
  }

  void put(const buffer_t &k, u32 v) {
    buffer_t value(4);
    value.set(0, 4, v);

    for (Table &table : tables) {
      LOG_DEBUG("[%s] Put key %s value %u", table.get_name().c_str(), k.to_string().c_str(), v);

      const std::vector<table_action_t> &actions = table.get_actions();
      assert(actions.size() == 1);

      const table_action_t &set_param_action = actions[0];
      table.add_or_mod_entry(k, set_param_action.name, {value});
    }

    cache[k] = v;
  }

  void del(const buffer_t &k) {
    auto found_it = cache.find(k);
    if (found_it == cache.end()) {
      return;
    }

    for (Table &table : tables) {
      LOG_DEBUG("[%s] Free key %s", table.get_name().c_str(), k.to_string().c_str());
      table.del_entry(k);
    }

    cache.erase(found_it);
  }

  void dump() const {
    std::stringstream ss;
    dump(ss);
    LOG("%s", ss.str().c_str());
  }

  void dump(std::ostream &os) const {
    os << "================================================\n";
    os << "Guarded Map Table Cache:\n";
    for (const auto &[k, v] : cache) {
      os << "  key=" << k << " value=" << v << "\n";
    }
    os << "================================================\n";

    for (const Table &table : tables) {
      table.dump(os);
    }
  }

private:
  static void expiration_callback(const bf_rt_target_t &dev_tgt, const bfrt::BfRtTableKey *key, void *cookie) {
    cfg.begin_dataplane_notification_transaction();

    FCFSCachedTable *fcfs_ct = reinterpret_cast<FCFSCachedTable *>(cookie);
    assert(fcfs_ct && "Invalid cookie");

    const bfrt::BfRtTable *table;
    bf_status_t status = key->tableGet(&table);
    ASSERT_BF_STATUS(status);

    std::string table_name;
    status = table->tableNameGet(&table_name);
    ASSERT_BF_STATUS(status);

    buffer_t key_buffer;
    bool target_table_found = false;
    for (const Table &target_table : fcfs_ct->tables) {
      if (target_table.get_full_name() == table_name) {
        target_table_found = true;
        key_buffer         = target_table.get_key_value(key);
        break;
      }
    }

    if (!target_table_found) {
      ERROR("Target table %s not found", table_name.c_str());
    }

    fcfs_ct->del(key_buffer);

    cfg.commit_dataplane_notification_transaction();
  }

  static bf_status_t digest_callback(const bf_rt_target_t &bf_rt_tgt, const std::shared_ptr<bfrt::BfRtSession> session,
                                     std::vector<std::unique_ptr<bfrt::BfRtLearnData>> learn_data, bf_rt_learn_msg_hdl *const learn_msg_hdl,
                                     const void *cookie) {
    // Ugh... But what choice do we have? Why the hell have they decided to enforce const on the cookie?
    FCFSCachedTable *fcfs_ct = const_cast<FCFSCachedTable *>(reinterpret_cast<const FCFSCachedTable *>(cookie));
    assert(fcfs_ct && "Invalid cookie");

    cfg.begin_dataplane_notification_transaction();

    for (const std::unique_ptr<bfrt::BfRtLearnData> &data_entry : learn_data) {
      const buffer_t diggest_buffer = fcfs_ct->digest.get_value(data_entry.get());

      const bytes_t hash_size_bytes = (fcfs_ct->cache_hash_size + 7) / 8;
      const u32 hash_mask           = (1 << fcfs_ct->cache_hash_size) - 1;

      const buffer_t key = diggest_buffer.get_slice(0, fcfs_ct->key_size);
      const u32 value    = diggest_buffer.get(fcfs_ct->key_size, 4);
      const u32 hash     = diggest_buffer.get(fcfs_ct->key_size + 4, hash_size_bytes) & hash_mask;

      LOG_DEBUG("[%s] Digest callback invoked (data=%s key=%s value=%x hash=%x)", fcfs_ct->digest.get_name().c_str(),
                diggest_buffer.to_string(true).c_str(), key.to_string(true).c_str(), value, hash);

      if (fcfs_ct->cache.find(key) != fcfs_ct->cache.end()) {
        continue;
      }

      fcfs_ct->put(key, value);
      fcfs_ct->reg_alarm.set(hash, 0);
    }

    fcfs_ct->digest.notify_ack(learn_msg_hdl);

    cfg.commit_dataplane_notification_transaction();

    return BF_SUCCESS;
  }

  static std::vector<Table> build_tables(const std::vector<std::string> &table_names) {
    assert(!table_names.empty() && "Table name must not be empty");

    std::vector<Table> tables;
    for (const std::string &table_name : table_names) {
      tables.emplace_back(table_name);
    }

    return tables;
  }

  static u32 get_capacity(const std::vector<Table> &tables) {
    assert(!tables.empty() && "Tables must not be empty");

    const u32 capacity = tables.front().get_effective_capacity();
    for (const Table &table : tables) {
      assert(table.get_effective_capacity() == capacity);
    }

    return capacity;
  }

  static bytes_t get_key_size(const std::vector<Table> &tables) {
    assert(!tables.empty() && "Tables must not be empty");

    bytes_t key_size = 0;
    for (const table_field_t &field : tables.front().get_key_fields()) {
      key_size += field.size / 8;
    }

    for (const Table &table : tables) {
      bytes_t table_key_size = 0;
      for (const table_field_t &field : table.get_key_fields()) {
        table_key_size += field.size / 8;
      }
      assert(table_key_size == key_size);
    }

    return key_size;
  }

  static bits_t get_cache_hash_size(u32 cache_capacity) {
    // We need to store the hash used in the cache register.
    // The hash is a number between 0 and cache_capacity-1, so we need enough bits to store that.
    // We round up to the nearest byte.
    return static_cast<bits_t>(std::ceil(std::log2(cache_capacity)));
  }
};

} // namespace sycon