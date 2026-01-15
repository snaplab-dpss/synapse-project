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

namespace sycon {

class FCFSCachedTable : public SynapseDS {
private:
  std::unordered_map<buffer_t, u32, buffer_hash_t> cache;
  std::unordered_map<buffer_t, std::unordered_set<std::string>, buffer_hash_t> expirations_per_key;
  std::unordered_map<bf_dev_pipe_t, u32> tails_per_pipe;
  std::unordered_map<bf_dev_pipe_t, u32> expected_head_per_pipe;
  std::vector<u32> control_plane_free_indices;
  std::unordered_map<bf_dev_pipe_t, u16> estimation_total_available_data_plane_indices_per_pipe;

  std::vector<Table> tables;
  Register reg_liveness;
  Register reg_integer_allocator_head;
  Register reg_integer_allocator_tail;
  Register reg_integer_allocator_indexes;
  std::vector<Register> regs_index_to_key;
  Digest digest;

  const u16 pipelines;
  const u32 capacity;
  const u32 cache_capacity;
  const bytes_t key_size;
  const bits_t cache_hash_size;
  const CRC32 crc32;

public:
  FCFSCachedTable(const std::string &_name, const std::vector<std::string> &table_names, const std::string &reg_liveness_name,
                  const std::string &reg_integer_allocator_head_name, const std::string &reg_integer_allocator_tail_name,
                  const std::string &reg_integer_allocator_indexes_name, const std::vector<std::string> &regs_index_to_key,
                  const std::string &digest_name, u32 minimum_controller_indices, std::optional<time_ms_t> timeout = std::nullopt)
      : SynapseDS(_name), tables(build_tables(table_names)), reg_liveness(reg_liveness_name),
        reg_integer_allocator_head(reg_integer_allocator_head_name), reg_integer_allocator_tail(reg_integer_allocator_tail_name),
        reg_integer_allocator_indexes(reg_integer_allocator_indexes_name), regs_index_to_key(build_regs_index_to_key(regs_index_to_key)),
        digest(digest_name), pipelines(get_pipelines(reg_integer_allocator_indexes)),
        capacity(get_capacity(reg_integer_allocator_indexes, tables, pipelines)), cache_capacity(reg_liveness.get_capacity()),
        key_size(get_key_size(tables)), cache_hash_size(get_cache_hash_size(cache_capacity)), crc32() {
    std::unordered_map<bf_dev_pipe_t, std::vector<u32>> control_plane_free_indices_per_pipe;

    if (minimum_controller_indices % 4 != 0) {
      minimum_controller_indices += 4 - (minimum_controller_indices % 4);
      LOG_DEBUG("WARNING: Total controller indices is not multiple of 4, rounding up to nearest multiple of 4: %u", minimum_controller_indices);
    }

    const u32 capacity_per_pipe               = capacity / pipelines;
    const u32 min_controller_indices_per_pipe = minimum_controller_indices / pipelines;
    const u32 tail                            = std::min(capacity_per_pipe - min_controller_indices_per_pipe, cache_capacity);

    LOG_DEBUG("FCFS Cached Table initialized with total capacity %u, cache capacity %u, pipelines %u", capacity, cache_capacity, pipelines);
    LOG_DEBUG("Minimum controller indices per pipe: %u", min_controller_indices_per_pipe);
    LOG_DEBUG("Tail per pipe: %u", tail);
    LOG_DEBUG("Total controller indices: %u", capacity - (tail * pipelines));

    for (u16 pipe_id = 0; pipe_id < pipelines; pipe_id++) {
      tails_per_pipe[pipe_id]         = tail;
      expected_head_per_pipe[pipe_id] = 0;
      reg_integer_allocator_head.set(0, 0, pipe_id);
      reg_integer_allocator_tail.set(0, tails_per_pipe[pipe_id], pipe_id);
      estimation_total_available_data_plane_indices_per_pipe[pipe_id] = 0;
      for (u32 i = 0; i < capacity_per_pipe; i++) {
        const u32 index = capacity_per_pipe * pipe_id + i;
        if (i < tails_per_pipe[pipe_id]) {
          reg_integer_allocator_indexes.set(i, index, pipe_id);
          estimation_total_available_data_plane_indices_per_pipe[pipe_id]++;
        } else {
          control_plane_free_indices_per_pipe[pipe_id].push_back(index);
        }
      }
    }

    auto terminating_condition = [&control_plane_free_indices_per_pipe]() {
      for (const auto &[pipe_id, indices] : control_plane_free_indices_per_pipe) {
        if (!indices.empty()) {
          return false;
        }
      }
      return true;
    };

    // Alternate among pipes to fill control plane free indices
    bf_dev_pipe_t pipe_id = 0;
    while (!terminating_condition()) {
      std::vector<u32> &indices = control_plane_free_indices_per_pipe[pipe_id];
      if (!indices.empty()) {
        const u32 index = *indices.begin();
        indices.erase(indices.begin());
        control_plane_free_indices.push_back(index);
      }
      pipe_id = (pipe_id + 1) % pipelines;
    }

    if (timeout.has_value()) {
      for (Table &table : tables) {
        table.set_notify_mode(timeout.value(), this, FCFSCachedTable::expiration_callback, true);
      }
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

  bool allocate_index_and_put(const buffer_t &k, u32 &new_index) {
    out_of_band_migration();

    if (cache.find(k) != cache.end()) {
      LOG_DEBUG("Key %s already present in cache with value %u, skipping allocate_index_and_put", k.to_string().c_str(), cache.at(k));
      return false;
    }

    if (cache.size() >= capacity) {
      LOG_DEBUG("WARNING: Attempted to add key to map, but map is full (capacity=%u)", capacity);
      return false;
    }

    if (control_plane_free_indices.empty()) {
      LOG_DEBUG("WARNING: Attempted to add key to map, but no free indices are available");
      return false;
    }

    new_index = control_plane_free_indices.back();
    control_plane_free_indices.pop_back();

    set_index_to_key_registers(new_index, k);

    return put(k, new_index);
  }

  bool put(const buffer_t &k, u32 v) {
    if (cache.size() >= capacity) {
      LOG_DEBUG("WARNING: Attempted to add key to map, but map is full (capacity=%u)", capacity);
      return false;
    }

    if (get(k, v)) {
      LOG_DEBUG("Key %s already present in cache with value %u, skipping put", k.to_string().c_str(), v);
      return false;
    }

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
    return true;
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
    mark_index_as_available(index);

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
  struct key_index_t {
    buffer_t key;
    u32 index;
  };

  key_index_t get_key_and_index_from_data_plane(u16 pipe_id, u32 head) {
    const u32 index = reg_integer_allocator_indexes.get_per_pipe(head)[pipe_id];

    buffer_t key(key_size);
    bytes_t offset = 0;

    for (Register &reg_key : regs_index_to_key) {
      const bytes_t value_size = reg_key.get_value_size() / 8;
      const u32 value          = reg_key.get_per_pipe(index)[pipe_id];
      key.set(offset, value_size, value);
      offset += value_size;
    }

    return {key, index};
  }

  void set_index_to_key_registers(u32 index, const buffer_t &key) {
    bytes_t offset = 0;
    for (Register &reg_key : regs_index_to_key) {
      const bytes_t value_size = reg_key.get_value_size() / 8;
      const u32 value          = static_cast<u32>(key.get(offset, value_size));
      reg_key.set(index, value);
      offset += value_size;
    }
  }

  void clear_index_to_key_registers(u32 index) {
    for (Register &reg_key : regs_index_to_key) {
      reg_key.set(index, 0);
    }
  }

  void mark_index_as_available(u32 index) {
    const u32 capacity_per_pipe = capacity / pipelines;
    const bf_dev_pipe_t pipe_id = index / capacity_per_pipe;
    if (estimation_total_available_data_plane_indices_per_pipe[pipe_id] < capacity_per_pipe) {
      const u16 new_tail = (tails_per_pipe[pipe_id] + 1) % capacity;
      LOG_DEBUG("[pipe=%u] Storing index (%u) at tail (%u) and setting tail = %u", pipe_id, index, tails_per_pipe[pipe_id], new_tail);
      estimation_total_available_data_plane_indices_per_pipe[pipe_id]++;
      reg_integer_allocator_indexes.set(tails_per_pipe[pipe_id], index, pipe_id);
      reg_integer_allocator_tail.set(0, tails_per_pipe[pipe_id], pipe_id);
      tails_per_pipe[pipe_id] = new_tail;
    } else {
      LOG_DEBUG("[pipe=%u] Not storing index (%u) as dataplane index allocator is full, adding to control plane free indices", pipe_id, index);
      control_plane_free_indices.push_back(index);
    }
  }

  void out_of_band_migration() {
    const std::vector<u32> heads = reg_integer_allocator_head.get_per_pipe(0);
    for (u16 pipe_id = 0; pipe_id < pipelines; pipe_id++) {
      for (u32 head = expected_head_per_pipe[pipe_id]; head != heads[pipe_id]; head = (head + 1) % capacity) {
        LOG_DEBUG("Out-of-band migration on pipe %u for head %u (current data plane head %u)", pipe_id, head, heads[pipe_id]);
        const key_index_t missed_key_index = get_key_and_index_from_data_plane(pipe_id, head);
        migrate_to_tables(missed_key_index.key, missed_key_index.index);
      }
      expected_head_per_pipe[pipe_id] = heads[pipe_id];
    }
  }

  void migrate_to_tables(const buffer_t &key, u32 index) {
    const u32 capacity_per_pipe = capacity / pipelines;
    const u16 pipe_id           = index / capacity_per_pipe;

    if (put(key, index)) {
      estimation_total_available_data_plane_indices_per_pipe[pipe_id]--;
    }
  }

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

  static bf_status_t digest_callback(const bf_rt_target_t &bf_rt_tgt, const std::shared_ptr<bfrt::BfRtSession> session,
                                     std::vector<std::unique_ptr<bfrt::BfRtLearnData>> learn_data, bf_rt_learn_msg_hdl *const learn_msg_hdl,
                                     const void *cookie) {
    // Ugh... But what choice do we have? Why the hell have they decided to enforce const on the cookie?
    FCFSCachedTable *fcfs_ct = const_cast<FCFSCachedTable *>(reinterpret_cast<const FCFSCachedTable *>(cookie));
    assert(fcfs_ct && "Invalid cookie");

    cfg.begin_dataplane_notification_transaction();

    for (const std::unique_ptr<bfrt::BfRtLearnData> &data_entry : learn_data) {
      const buffer_t diggest_buffer = fcfs_ct->digest.get_value(data_entry.get());

      const u32 head     = diggest_buffer.get(0, 4);
      const buffer_t key = diggest_buffer.get_slice(4, fcfs_ct->key_size);
      const u32 index    = diggest_buffer.get(4 + fcfs_ct->key_size, 4);

      LOG_DEBUG("[%s] Digest callback invoked (data=%s key=%s index=0x%x hash=0x%x)", fcfs_ct->digest.get_name().c_str(),
                diggest_buffer.to_string(true).c_str(), key.to_string(true).c_str(), index,
                fcfs_ct->crc32.hash(key) & ((1 << fcfs_ct->cache_hash_size) - 1));

      if (fcfs_ct->cache.find(key) != fcfs_ct->cache.end()) {
        LOG_DEBUG("Key %s already present in cache with value %u, skipping digest callback", key.to_string().c_str(), fcfs_ct->cache.at(key));
        continue;
      }

      fcfs_ct->migrate_to_tables(key, index);

      const u16 pipe_id = index / (fcfs_ct->capacity / fcfs_ct->pipelines);

      if (head != fcfs_ct->expected_head_per_pipe[pipe_id]) {
        LOG_DEBUG("******* WARNING: Digest integer allocator head (%u) does not match expected head (%u) on pipe %u. Fixing this.", head,
                  fcfs_ct->expected_head_per_pipe[pipe_id], pipe_id);

        for (u32 missed_head = fcfs_ct->expected_head_per_pipe[pipe_id]; missed_head != head; missed_head = (missed_head + 1) % fcfs_ct->capacity) {
          const key_index_t missed_key_index = fcfs_ct->get_key_and_index_from_data_plane(pipe_id, missed_head);
          fcfs_ct->migrate_to_tables(missed_key_index.key, missed_key_index.index);
        }
      }

      fcfs_ct->expected_head_per_pipe[pipe_id] = (head + 1) % fcfs_ct->capacity;
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

  static std::vector<Register> build_regs_index_to_key(const std::vector<std::string> &regs_index_to_key) {
    assert(!regs_index_to_key.empty() && "Register keys names must not be empty");

    std::vector<Register> regs_keys;
    for (const std::string &reg_key_name : regs_index_to_key) {
      regs_keys.emplace_back(reg_key_name);
    }

    return regs_keys;
  }

  static u32 get_pipelines(Register &reg_integer_allocator_indexes) { return reg_integer_allocator_indexes.get_per_pipe(0).size(); }

  static u32 get_capacity(const Register &reg_integer_allocator_indexes, const std::vector<Table> &tables, u32 pipelines) {
    const u32 capacity = reg_integer_allocator_indexes.get_capacity();

    for (const Table &table : tables) {
      assert(table.get_effective_capacity() >= capacity);
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
