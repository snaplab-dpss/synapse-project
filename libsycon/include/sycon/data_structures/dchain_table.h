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
  DchainTable(const std::string &control_name, const std::vector<std::string> &table_names) : capacity(0) {
    assert(!table_names.empty() && "Table name must not be empty");

    for (const std::string &table_name : table_names) {
      tables.emplace_back(control_name, table_name);
      capacity = tables.back().get_capacity();
    }

    for (const Table &table : tables) {
      assert(table.get_capacity() == capacity);
    }

    for (u32 i = 0; i < capacity; i++) {
      free_indexes.insert(i);
    }
  }

  bool is_index_allocated(u32 index) const {
    auto found_it = cache.find(index);
    if (found_it == cache.end()) {
      return false;
    }

    return true;
  }

  bool allocate_new_index(u32 &index) {
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
};

} // namespace sycon