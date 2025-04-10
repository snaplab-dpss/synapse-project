#pragma once

#include <array>
#include <optional>
#include <unordered_set>

#include "../constants.h"
#include "../primitives/table.h"
#include "../time.h"
#include "../field.h"
#include "synapse_ds.h"

namespace sycon {

class VectorTable : public SynapseDS {
private:
  std::vector<buffer_t> cache;
  std::vector<Table> tables;
  u32 capacity;
  bits_t value_size;

public:
  VectorTable(const std::string &_name, const std::vector<std::string> &table_names) : SynapseDS(_name), capacity(0), value_size(0) {
    assert(!table_names.empty() && "Table name must not be empty");

    for (const std::string &table_name : table_names) {
      tables.emplace_back(table_name);
      capacity = tables.back().get_effective_capacity();

      const std::vector<sycon::table_action_t> &actions = tables.back().get_actions();
      assert(actions.size() == 1);
      const table_action_t &action = actions[0];
      for (const table_field_t &field : action.data_fields) {
        value_size += field.size;
      }
    }

    for (const Table &table : tables) {
      assert(table.get_effective_capacity() == capacity);
    }

    buffer_t value(value_size / 8);
    cache.resize(capacity, value);
  }

  void read(u32 index, buffer_t &v) const {
    assert(index < capacity && "Index out of bounds");
    v = cache.at(index);
  }

  void write(u32 index, const buffer_t &value) {
    assert(index < capacity && "Index out of bounds");

    buffer_t key(4);
    key.set(0, 4, index);

    for (Table &table : tables) {
      LOG_DEBUG("[%s] Write index %u value %s", table.get_name().c_str(), index, value.to_string().c_str());

      const std::vector<table_action_t> &actions = table.get_actions();
      assert(actions.size() == 1);

      const table_action_t &set_param_action = actions[0];
      table.add_or_mod_entry(key, set_param_action.name, {value});
    }

    cache.at(index) = value;
  }

  void dump() const {
    std::stringstream ss;
    dump(ss);
    LOG("%s", ss.str().c_str());
  }

  void dump(std::ostream &os) const {
    os << "================================================\n";
    os << "Vector Table Cache:\n";
    for (u32 i = 0; i < capacity; i++) {
      os << "  index=" << i << " value=" << cache.at(i) << "\n";
    }
    os << "================================================\n";

    for (const Table &table : tables) {
      table.dump(os);
    }
  }
};

} // namespace sycon