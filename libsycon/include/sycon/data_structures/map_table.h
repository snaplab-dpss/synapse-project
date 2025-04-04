#pragma once

#include <array>
#include <optional>
#include <unordered_set>

#include "../config.h"
#include "../constants.h"
#include "../primitives/table.h"
#include "../time.h"
#include "../field.h"
#include "synapse_ds.h"

namespace sycon {

class MapTable : public SynapseDS {
private:
  std::unordered_map<buffer_t, u32, buffer_hash_t> cache;
  std::vector<Table> tables;
  u32 capacity;
  bits_t key_size;

  // For the transactional rollback/commit
  std::unordered_map<buffer_t, u32, buffer_hash_t> new_entries_on_hold;
  std::unordered_map<buffer_t, u32, buffer_hash_t> modified_entries_backup;

public:
  MapTable(const std::string &_name, const std::vector<std::string> &table_names, std::optional<time_ms_t> timeout = std::nullopt)
      : SynapseDS(_name), capacity(0), key_size(0) {
    assert(!table_names.empty() && "Table name must not be empty");

    for (const std::string &table_name : table_names) {
      tables.emplace_back(table_name);
      capacity = tables.back().get_capacity();
      for (const table_field_t &field : tables.back().get_key_fields()) {
        key_size += field.size;
      }
    }

    for (const Table &table : tables) {
      assert(table.get_capacity() == capacity);
    }

    if (timeout.has_value()) {
      if (timeout.value() < TOFINO_MODEL_MIN_EXPIRATION_TIME) {
        LOG_DEBUG("Warning: Timeout value is too low, setting to minimum value %lu ms", TOFINO_MODEL_MIN_EXPIRATION_TIME);
        timeout = TOFINO_MODEL_MIN_EXPIRATION_TIME;
      }
      Table &chosen_expiration_table = tables.front();
      chosen_expiration_table.set_notify_mode(timeout.value(), this, MapTable::expiration_callback, true);
    }
  }

  bool get(const buffer_t &k, u32 &v) const {
    auto found_it = cache.find(k);
    if (found_it == cache.end()) {
      return false;
    }

    v = found_it->second;
    return true;
  }

  void put(const buffer_t &k, u32 v, bool &duplicate_request_detected) {
    duplicate_request_detected = false;

    buffer_t value(4);
    value.set(0, 4, v);

    auto found_it = cache.find(k);

    if (found_it != cache.end()) {
      duplicate_request_detected = true;
      return;
    }

    if (found_it == cache.end()) {
      new_entries_on_hold[k] = v;
    } else {
      auto modified_entry_it = modified_entries_backup.find(k);
      if (modified_entry_it == modified_entries_backup.end()) {
        modified_entries_backup[k] = v;
      }
    }

    for (Table &table : tables) {
      LOG_DEBUG("[%s] Put key %s value %u", table.get_name().c_str(), k.to_string().c_str(), v);

      const std::vector<table_action_t> &actions = table.get_actions();
      assert(actions.size() == 1);

      const table_action_t &set_param_action = actions[0];
      table.add_or_mod_entry(k, set_param_action.name, {value});
    }

    cache[k] = v;
  }

  void del(const buffer_t &k, bool &duplicate_request_detected) {
    duplicate_request_detected = false;

    auto found_it = cache.find(k);
    if (found_it == cache.end()) {
      duplicate_request_detected = true;
      return;
    }

    auto modified_entry_it = modified_entries_backup.find(k);
    if (modified_entry_it == modified_entries_backup.end()) {
      modified_entries_backup[k] = found_it->second;
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
    os << "Map Table Cache:\n";
    for (const auto &[k, v] : cache) {
      os << "  key=" << k << " value=" << v << "\n";
    }
    os << "================================================\n";

    for (const Table &table : tables) {
      table.dump(os);
    }
  }

  virtual void rollback() override final {
    if (new_entries_on_hold.empty() && modified_entries_backup.empty()) {
      return;
    }

    LOG_DEBUG("[%s] Aborted tx: rolling back state", name.c_str());

    for (const auto &[k, v] : new_entries_on_hold) {
      cache.erase(k);
    }

    for (const auto &[k, v] : modified_entries_backup) {
      cache[k] = v;
    }

    new_entries_on_hold.clear();
    modified_entries_backup.clear();
  }

  virtual void commit() override final {
    new_entries_on_hold.clear();
    modified_entries_backup.clear();
  }

private:
  static void expiration_callback(const bf_rt_target_t &dev_tgt, const bfrt::BfRtTableKey *key, void *cookie) {
    cfg.begin_transaction();

    MapTable *map_table = reinterpret_cast<MapTable *>(cookie);
    assert(map_table && "Invalid cookie");

    const bfrt::BfRtTable *table;
    bf_status_t status = key->tableGet(&table);
    ASSERT_BF_STATUS(status);

    std::string table_name;
    status = table->tableNameGet(&table_name);
    ASSERT_BF_STATUS(status);

    buffer_t key_buffer;
    bool target_table_found = false;
    for (const Table &target_table : map_table->tables) {
      if (target_table.get_full_name() == table_name) {
        target_table_found = true;
        key_buffer         = target_table.get_key_value(key);
        break;
      }
    }

    if (!target_table_found) {
      ERROR("Target table %s not found", table_name.c_str());
    }

    bool duplicate_request_detected;
    map_table->del(key_buffer, duplicate_request_detected);

    if (duplicate_request_detected) {
      map_table->rollback();
      cfg.abort_transaction();
    } else {
      map_table->commit();
      cfg.commit_transaction();
    }
  }
};

} // namespace sycon