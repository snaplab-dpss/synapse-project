#pragma once

#include <array>
#include <optional>
#include <unordered_set>

#include "../config.h"
#include "../constants.h"
#include "../primitives/table.h"
#include "../primitives/register.h"
#include "../time.h"
#include "../field.h"
#include "synapse_ds.h"

namespace sycon {

class GuardedMapTable : public SynapseDS {
private:
  std::unordered_map<buffer_t, u32, buffer_hash_t> cache;
  std::unordered_map<buffer_t, std::unordered_set<std::string>, buffer_hash_t> expirations_per_key;
  bool guard_cache;

  std::vector<Table> tables;
  std::optional<Register> guard;

  const u32 capacity;

public:
  GuardedMapTable(const std::string &_name, const std::vector<std::string> &table_names, const std::string &guard_name,
                  std::optional<time_ms_t> timeout = std::nullopt)
      : SynapseDS(_name), tables(build_tables(table_names)), guard(conditional_build_guard(guard_name)), capacity(get_capacity(tables)) {
    if (guard.has_value()) {
      guard_allow();
    }

    if (timeout.has_value()) {
      for (Table &table : tables) {
        table.set_notify_mode(timeout.value(), this, GuardedMapTable::expiration_callback, true);
      }
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

    if (cache.size() >= capacity) {
      guard_disallow();
    }
  }

  bool guard_check() const { return guard_cache; }

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

    if (guard.has_value()) {
      guard_allow();
    }
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

    GuardedMapTable *map_table = reinterpret_cast<GuardedMapTable *>(cookie);
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

    map_table->expirations_per_key[key_buffer].insert(table_name);
    if (map_table->expirations_per_key[key_buffer].size() == map_table->tables.size()) {
      map_table->del(key_buffer);
      map_table->expirations_per_key.erase(key_buffer);
    }

    cfg.commit_dataplane_notification_transaction();
  }

  void guard_allow() {
    assert(guard.has_value() && "Guard register not initialized");
    guard->set(0, 1);
    guard_cache = true;
  }

  void guard_disallow() {
    assert(guard.has_value() && "Guard register not initialized");
    guard->set(0, 0);
    guard_cache = false;
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

  static std::optional<Register> conditional_build_guard(const std::string &name) {
    const bfrt::BfRtTable *table;
    if (cfg.info->bfrtTableFromNameGet(name, &table) != BF_SUCCESS) {
      return {};
    }

    std::optional<Register> guard;
    guard.emplace(name);
    return guard;
  }
};

} // namespace sycon