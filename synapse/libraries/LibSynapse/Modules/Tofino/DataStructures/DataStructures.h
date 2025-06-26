#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/Table.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Register.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Meter.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Hash.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Digest.h>
#include <LibSynapse/Modules/Tofino/DataStructures/FCFSCachedTable.h>
#include <LibSynapse/Modules/Tofino/DataStructures/HHTable.h>
#include <LibSynapse/Modules/Tofino/DataStructures/CountMinSketch.h>
#include <LibSynapse/Modules/Tofino/DataStructures/LPM.h>
#include <LibSynapse/Modules/Tofino/DataStructures/MapTable.h>
#include <LibSynapse/Modules/Tofino/DataStructures/VectorTable.h>
#include <LibSynapse/Modules/Tofino/DataStructures/DchainTable.h>
#include <LibSynapse/Modules/Tofino/DataStructures/VectorRegister.h>
#include <LibSynapse/Modules/Tofino/DataStructures/GuardedMapTable.h>

#include <LibCore/Debug.h>

namespace LibSynapse {
namespace Tofino {

class DataStructures {
private:
  std::vector<std::unique_ptr<DS>> data;
  std::unordered_map<addr_t, std::unordered_set<DS *>> data_per_obj;
  std::unordered_map<DS_ID, DS *> data_per_id;

public:
  DataStructures() {}

  DataStructures(const DataStructures &other) {
    for (const auto &[addr, dss] : other.data_per_obj) {
      for (DS *ds : dss) {
        DS *clone = ds->clone();
        data.emplace_back(clone);
        data_per_obj[addr].insert(clone);
        data_per_id[clone->id] = clone;
      }
    }
  }

  bool has(addr_t addr) const { return data_per_obj.find(addr) != data_per_obj.end(); }
  bool has(DS_ID id) const { return data_per_id.find(id) != data_per_id.end(); }

  const std::unordered_set<DS *> &get_ds(addr_t addr) const {
    auto found_it = data_per_obj.find(addr);
    if (found_it == data_per_obj.end()) {
      panic("Data structure not found with addr %lu", addr);
    }
    return found_it->second;
  }

  const std::unordered_map<addr_t, std::unordered_set<DS *>> &get_data_per_obj() const { return data_per_obj; }
  const std::unordered_map<DS_ID, DS *> &get_data_per_id() const { return data_per_id; }

  template <class DS_T> const DS_T *get_single_ds(addr_t addr) const {
    const std::unordered_set<DS *> &ds = get_ds(addr);
    assert(ds.size() == 1 && "Expected exactly one DS");
    return dynamic_cast<const DS_T *>(*ds.begin());
  }

  const DS *get_ds_from_id(DS_ID id) const {
    auto it = data_per_id.find(id);
    assert(it != data_per_id.end() && "Data structure not found");
    return it->second;
  }

  // FIXME: this should be a unique_ptr
  void save(addr_t addr, DS *ds) {
    auto found_it = data_per_id.find(ds->id);

    if (found_it != data_per_id.end()) {
      DS *old = found_it->second;
      assert(old->id == ds->id && "Data structure ID mismatch");
      data.erase(std::remove_if(data.begin(), data.end(), [old](const std::unique_ptr<DS> &d) { return d.get() == old; }), data.end());
      data_per_id.erase(ds->id);
      data_per_obj[addr].erase(old);
    }

    data.emplace_back(ds);
    data_per_obj[addr].insert(ds);
    data_per_id[ds->id] = ds;
  }
};

} // namespace Tofino
} // namespace LibSynapse