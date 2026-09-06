#pragma once

#include <array>
#include <optional>
#include <unordered_set>

#include "synapse_ds.h"
#include "../primitives/table.h"
#include "../primitives/register.h"
#include "../primitives/digest.h"
#include "../time.h"
#include "../field.h"
#include "../hash.h"
#include <algorithm>

namespace sycon {

class FCFSCachedTable : public SynapseDS {
private:
  std::unordered_map<buffer_t, u32, buffer_hash_t> cache;
  std::unordered_map<buffer_t, std::unordered_set<std::string>, buffer_hash_t> expirations_per_key;
  std::vector<u32> control_plane_free_indices;
  std::unordered_set<u32> control_plane_allocated_indices;

  std::vector<Table> tables;
  Register reg_liveness;
  std::vector<Register> reg_cached_keys;

  const u32 capacity;
  const u32 cache_capacity;
  const bytes_t key_size;
  const CRC32 crc32;

public:
  FCFSCachedTable(const std::string &_name, const std::vector<std::string> &table_names, const std::string &reg_liveness_name,
                  const std::vector<std::string> &reg_cached_keys_names, std::optional<time_ms_t> timeout = std::nullopt)
      : SynapseDS(_name), tables(build_tables(table_names)), reg_liveness(reg_liveness_name),
        reg_cached_keys(build_regs_cached_keys(reg_cached_keys_names)), capacity(get_capacity(reg_liveness, tables)),
        cache_capacity(get_cache_capacity(reg_cached_keys)), key_size(get_key_size(tables)) {
    assert(cache_capacity < capacity && "Cache capacity must be less than the total capacity, otherwise we can't handle hash collisions");

    cache.reserve(capacity);
    expirations_per_key.reserve(capacity);
    control_plane_free_indices.reserve(capacity - cache_capacity);

    for (u32 i = cache_capacity; i < capacity; i++) {
      control_plane_free_indices.push_back(i);
    }

    if (timeout.has_value()) {
      for (Table &table : tables) {
        table.set_notify_mode(timeout.value(), this, FCFSCachedTable::expiration_callback, true);
      }
    }
  }

  bool get(const buffer_t &k, u32 &index) {
    auto found_it = cache.find(k);
    if (found_it != cache.end()) {
      index = found_it->second;
      return true;
    }
    return get_from_dataplane_cache(k, index);
  }

  // The dataplane inserts into its own cache (key registers at a CRC32 slot) without telling
  // the controller, so `cache` only knows the keys the controller put itself. A controller
  // replay of map_get for a key the dataplane admitted must therefore mirror the P4 lookup:
  // read the key registers at crc32(key) truncated to the cache index width and accept the
  // slot if some pipe holds the key there (registers are per pipe). Liveness is not checked:
  // the dataplane only hands the packet up after finding the entry alive.
  bool get_from_dataplane_cache(const buffer_t &k, u32 &index) {
    const u32 slot = crc32.hash(k) & (cache_capacity - 1);
    std::vector<bool> match_per_pipe;
    for (size_t i = 0; i < reg_cached_keys.size(); i++) {
      const bytes_t offset = 4 * i;
      const bytes_t width  = std::min<bytes_t>(4, key_size - offset);
      const u64 expected   = k.get(offset, width);
      const std::vector<u32> values_per_pipe = reg_cached_keys[i].get_per_pipe(slot);
      if (match_per_pipe.empty()) {
        match_per_pipe.assign(values_per_pipe.size(), true);
      }
      for (size_t pipe = 0; pipe < values_per_pipe.size(); pipe++) {
        if (values_per_pipe[pipe] != expected) {
          match_per_pipe[pipe] = false;
        }
      }
    }
    for (bool match : match_per_pipe) {
      if (match) {
        index = slot;
        return true;
      }
    }
    return false;
  }

  bool is_index_allocated(u32 index) const { return control_plane_allocated_indices.find(index) != control_plane_allocated_indices.end(); }

  bool allocate_index_and_put(const buffer_t &k, u32 &new_index) {
    if (control_plane_free_indices.empty()) {
      LOG_DEBUG("WARNING: Attempted to add key to map, but no free indices are available");
      return false;
    }

    new_index = control_plane_free_indices.back();
    control_plane_free_indices.pop_back();
    control_plane_allocated_indices.insert(new_index);

    put(k, new_index);
    return true;
  }

  void put(const buffer_t &k, u32 v) {
    buffer_t value(4);
    value.set(0, 4, v);

    for (Table &table : tables) {
      LOG_DEBUG("[%s] Put key %s", table.get_name().c_str(), k.to_string().c_str());

      const std::vector<table_action_t> &actions = table.get_actions();
      assert(actions.size() == 1);

      const table_action_t &set_param_action = actions[0];
      table.add_or_mod_entry(k, set_param_action.name, {value});
    }

    cache.emplace(k, v);
    reg_liveness.set(v, 0xffffffff);
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

    const u32 index = found_it->second;
    if (index >= cache_capacity) {
      control_plane_free_indices.push_back(index);
      control_plane_allocated_indices.erase(index);
      reg_liveness.set(index, 0);
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
    os << "FCFS Cached Table cache:\n";
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

    fcfs_ct->expirations_per_key[key_buffer].insert(table_name);
    if (fcfs_ct->expirations_per_key[key_buffer].size() == fcfs_ct->tables.size()) {
      fcfs_ct->del(key_buffer);
      fcfs_ct->expirations_per_key.erase(key_buffer);
    }

    cfg.commit_dataplane_notification_transaction();
  }

  static std::vector<Table> build_tables(const std::vector<std::string> &table_names) {
    assert(!table_names.empty() && "Table name must not be empty");

    std::vector<Table> tables;
    for (const std::string &table_name : table_names) {
      tables.emplace_back(table_name);
    }

    return tables;
  }

  static u32 get_capacity(const Register &reg_liveness, const std::vector<Table> &tables) {
    const u32 capacity = reg_liveness.get_capacity();

    for (const Table &table : tables) {
      assert(table.get_effective_capacity() >= capacity);
    }

    return capacity;
  }

  static u32 get_cache_capacity(const std::vector<Register> &reg_cached_keys) {
    assert(!reg_cached_keys.empty() && "Cached keys registers must not be empty");
    const u32 capacity = reg_cached_keys.front().get_capacity();

    for (const Register &reg : reg_cached_keys) {
      assert(reg.get_capacity() == capacity);
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

  static std::vector<Register> build_regs_cached_keys(const std::vector<std::string> &regs_cached_keys) {
    assert(!regs_cached_keys.empty() && "Register keys names must not be empty");

    std::vector<Register> regs_keys;
    for (const std::string &reg_name : regs_cached_keys) {
      regs_keys.emplace_back(reg_name);
    }

    return regs_keys;
  }
};

} // namespace sycon
