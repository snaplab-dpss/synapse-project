#pragma once

#include <array>
#include <optional>
#include <unordered_set>

#include "../constants.h"
#include "../primitives/table.h"
#include "../time.h"
#include "../field.h"

namespace sycon {

class VectorTable {
private:
  std::unordered_map<u32, buffer_t> cache;
  std::vector<Table> tables;
  u32 capacity;
  bits_t value_size;

public:
  VectorTable(const std::string &control_name, const std::vector<std::string> &table_names) : capacity(0), value_size(0) {
    assert(!table_names.empty() && "Table name must not be empty");

    for (const std::string &table_name : table_names) {
      tables.emplace_back(control_name, table_name);
      capacity = tables.back().get_capacity();

      const std::vector<sycon::table_action_t> &actions = tables.back().get_actions();
      assert(actions.size() == 1);
      const table_action_t &action = actions[0];
      for (const table_field_t &field : action.data_fields) {
        value_size += field.size;
      }
    }

    for (const Table &table : tables) {
      assert(table.get_capacity() == capacity);
    }
  }

  bool get(u32 k, buffer_t &v) const {
    auto found_it = cache.find(k);
    if (found_it == cache.end()) {
      return false;
    }

    v = found_it->second;
    return true;
  }

  void put(u32 k, const buffer_t &v) {
    buffer_t key(4);
    key.set(0, 4, k);

    for (Table &table : tables) {
      const std::vector<table_action_t> &actions = table.get_actions();
      assert(actions.size() == 1);

      const table_action_t &set_param_action = actions[0];
      table.add_or_mod_entry(key, set_param_action.name, {v});
    }

    cache[k] = v;
  }

  void del(u32 k) {
    auto found_it = cache.find(k);
    if (found_it == cache.end()) {
      return;
    }

    buffer_t key(4);
    key.set(0, 4, k);

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
    os << "Vector Table Cache:\n";
    for (const auto &[k, v] : cache) {
      os << "  key=" << k << " value=" << v << "\n";
    }
    os << "================================================\n";

    for (const Table &table : tables) {
      table.dump(os);
    }
  }
};

} // namespace sycon