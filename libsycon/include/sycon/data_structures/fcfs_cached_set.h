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

class FCFSCachedSet : public SynapseDS {
private:
  std::unordered_set<buffer_t, buffer_hash_t> cache;
  std::unordered_map<buffer_t, std::unordered_set<std::string>, buffer_hash_t> expirations_per_key;

  std::vector<Table> tables;

  const u32 capacity;
  const bytes_t key_size;

public:
  FCFSCachedSet(const std::string &_name, const std::vector<std::string> &table_names, std::optional<time_ms_t> timeout = std::nullopt)
      : SynapseDS(_name), tables(build_tables(table_names)), capacity(get_capacity(tables)), key_size(get_key_size(tables)) {
    if (timeout.has_value()) {
      for (Table &table : tables) {
        table.set_notify_mode(timeout.value(), this, FCFSCachedSet::expiration_callback, true);
      }
    }
  }

  bool get(const buffer_t &k) const {
    auto found_it = cache.find(k);
    if (found_it == cache.end()) {
      return false;
    }
    return true;
  }

  void put(const buffer_t &k) {
    for (Table &table : tables) {
      LOG_DEBUG("[%s] Put key %s", table.get_name().c_str(), k.to_string().c_str());
      table.add_or_mod_entry(k);
    }

    cache.insert(k);
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

    FCFSCachedSet *fcfs_ct = reinterpret_cast<FCFSCachedSet *>(cookie);
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

  static u32 get_capacity(const std::vector<Table> &tables) {
    assert(!tables.empty() && "Tables must not be empty");
    const u32 capacity = tables.back().get_effective_capacity();

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
};

} // namespace sycon
