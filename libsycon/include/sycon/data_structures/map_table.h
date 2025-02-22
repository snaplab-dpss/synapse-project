#pragma once

#include <array>
#include <optional>
#include <unordered_set>

#include "../constants.h"
#include "../primitives/table.h"
#include "../time.h"
#include "../field.h"

namespace sycon {

class MapTable {
private:
  std::unordered_map<buffer_t, u32, buffer_hash_t> cache;
  std::vector<Table> tables;
  u32 capacity;
  bits_t key_size;

public:
  MapTable(const std::string &control_name, const std::vector<std::string> &table_names) : capacity(0), key_size(0) {
    assert(!table_names.empty() && "Table name must not be empty");

    for (const std::string &table_name : table_names) {
      tables.emplace_back(control_name, table_name);
      capacity = tables.back().get_capacity();
      for (const table_field_t &field : tables.back().get_key_fields()) {
        key_size += field.size;
      }
    }

    for (const Table &table : tables) {
      assert(table.get_capacity() == capacity);
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
    auto found_it  = cache.find(k);
    bool new_entry = found_it == cache.end();

    buffer_t value(4);
    value.set(0, 4, v);

    for (Table &table : tables) {
      const std::vector<table_action_t> &actions = table.get_actions();
      assert(actions.size() == 1);

      const table_action_t &set_param_action = actions[0];

      if (new_entry) {
        table.add_entry(k, set_param_action.name, {value});
      } else {
        table.mod_entry(k, set_param_action.name, {value});
      }
    }

    cache[k] = v;
  }

  void del(const buffer_t &k) {
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
    os << "Map Table Cache:\n";
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