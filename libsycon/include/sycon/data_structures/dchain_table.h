#pragma once

#include <array>
#include <optional>
#include <unordered_set>

#include "../constants.h"
#include "../primitives/table.h"
#include "../time.h"
#include "../field.h"

namespace sycon {

class DchainTable {
private:
  std::unordered_set<u32> cache;
  std::unordered_set<u32> free_indexes;
  std::vector<Table> tables;
  u32 capacity;

public:
  DchainTable(const std::string &control_name, const std::vector<std::string> &table_names, time_ms_t timeout) : capacity(0) {
    assert(!table_names.empty() && "Table name must not be empty");

    for (const std::string &table_name : table_names) {
      tables.emplace_back(control_name, table_name);
    }

    capacity = tables.back().get_capacity();

    for (u32 i = 0; i < capacity; i++) {
      free_indexes.insert(i);
    }

    for (Table &table : tables) {
      assert(table.get_capacity() == capacity);
      table.set_notify_mode(timeout, this, DchainTable::expiration_callback, true);
    }
  }

  bool is_index_allocated(u32 index) const {
    assert(index < capacity && "Invalid index");

    auto found_it = cache.find(index);
    if (found_it == cache.end()) {
      return false;
    }

    return true;
  }

  void refresh_index(u32 index) {
    assert(index < capacity && "Invalid index");

    auto found_it = cache.find(index);
    if (found_it == cache.end()) {
      return;
    }

    buffer_t key(4);
    key.set(0, 4, index);

    for (Table &table : tables) {
      table.mod_entry(key);
    }
  }

  bool allocate_new_index(u32 &index) {
    assert(index < capacity && "Invalid index");

    if (free_indexes.empty()) {
      return false;
    }

    index = *free_indexes.begin();
    free_indexes.erase(free_indexes.begin());

    buffer_t key(4);
    key.set(0, 4, index);

    for (Table &table : tables) {
      table.add_entry(key);
    }

    cache.insert(index);

    return true;
  }

  void free_index(u32 index) {
    assert(index < capacity && "Invalid index");

    auto found_it = cache.find(index);
    if (found_it == cache.end()) {
      return;
    }

    buffer_t key(4);
    key.set(0, 4, index);

    for (Table &table : tables) {
      table.del_entry(key);
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
    os << "Dchain Table Cache:\n";
    for (u32 k : cache) {
      os << "  key=" << k << "\n";
    }
    os << "================================================\n";

    for (const Table &table : tables) {
      table.dump(os);
    }
  }

private:
  static void expiration_callback(const bf_rt_target_t &dev_tgt, const bfrt::BfRtTableKey *key, void *cookie) {
    cfg.begin_transaction();

    DchainTable *dchain_table = reinterpret_cast<DchainTable *>(cookie);
    assert(dchain_table && "Invalid cookie");

    const bfrt::BfRtTable *table;
    bf_status_t status = key->tableGet(&table);
    ASSERT_BF_STATUS(status);

    std::string table_name;
    status = table->tableNameGet(&table_name);
    ASSERT_BF_STATUS(status);

    bool target_table_found = false;
    table_field_t key_field;
    for (const Table &target_table : dchain_table->tables) {
      if (target_table.get_full_name() == table_name) {
        target_table_found = true;
        assert(target_table.get_key_fields().size() == 1);
        key_field = target_table.get_key_fields()[0];
        break;
      }
    }

    if (!target_table_found) {
      ERROR("Target table %s not found", table_name.c_str());
    }

    u64 key_value;
    status = key->getValue(key_field.id, &key_value);
    ASSERT_BF_STATUS(status);

    DEBUG("[%s] Expiring index %lu", table_name.c_str(), key_value);

    u32 index = static_cast<u32>(key_value);
    dchain_table->free_index(index);

    cfg.end_transaction();
  }
};

} // namespace sycon