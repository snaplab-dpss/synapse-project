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
  }

  bool get(u32 k) const {
    auto found_it = cache.find(k);
    if (found_it == cache.end()) {
      return false;
    }

    return true;
  }

  void put(u32 k) {
    auto found_it  = cache.find(k);
    bool new_entry = found_it == cache.end();

    for (Table &table : tables) {
      if (new_entry) {
        table.add_entry(k);
      } else {
        table.mod_entry(k);
      }
    }

    cache.insert(k);
  }

  void del(u32 k) {
    auto found_it = cache.find(k);
    if (found_it == cache.end()) {
      return;
    }

    for (Table &table : tables) {
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