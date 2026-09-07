#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/Table.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Register.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Meter.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Hash.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Digest.h>
#include <LibSynapse/Modules/Tofino/DataStructures/ComputeAction.h>
#include <LibSynapse/Modules/Tofino/DataStructures/FCFSCachedTable.h>
#include <LibSynapse/Modules/Tofino/DataStructures/FCFSCachedSet.h>
#include <LibSynapse/Modules/Tofino/DataStructures/HHTable.h>
#include <LibSynapse/Modules/Tofino/DataStructures/CountMinSketch.h>
#include <LibSynapse/Modules/Tofino/DataStructures/LPM.h>
#include <LibSynapse/Modules/Tofino/DataStructures/MapTable.h>
#include <LibSynapse/Modules/Tofino/DataStructures/MapSetTable.h>
#include <LibSynapse/Modules/Tofino/DataStructures/VectorTable.h>
#include <LibSynapse/Modules/Tofino/DataStructures/DchainTable.h>
#include <LibSynapse/Modules/Tofino/DataStructures/VectorRegister.h>
#include <LibSynapse/Modules/Tofino/DataStructures/GuardedMapTable.h>
#include <LibSynapse/Modules/Tofino/DataStructures/CuckooHashTable.h>
#include <LibSynapse/Modules/Tofino/DataStructures/BloomFilter.h>

#include <LibCore/Debug.h>
#include <LibCore/Cow.h>

namespace LibSynapse {
namespace Tofino {

// The placed data structures of one execution plan. A data structure is never modified after
// it is saved (modules that want a changed one save a clone under the same id), so copies of
// this container -- one per speculation and per execution plan -- share the objects instead of
// cloning them; that is what keeps speculation affordable on plans with hundreds of tables.
class DataStructures {
private:
  struct Impl {
    std::vector<std::shared_ptr<DS>> data;
    std::unordered_map<addr_t, std::unordered_set<DS *>> data_per_obj; // FIXME: there should be only one DS per addr_t
    std::unordered_map<DS_ID, DS *> data_per_id;
  };

  // Shared between copies until one of them saves a data structure (copy-on-write).
  LibCore::Cow<Impl> impl;

  const std::vector<std::shared_ptr<DS>> &all() const { return impl->data; }
  const std::unordered_map<addr_t, std::unordered_set<DS *>> &objs() const { return impl->data_per_obj; }
  const std::unordered_map<DS_ID, DS *> &ids() const { return impl->data_per_id; }

public:
  DataStructures() {}

  DataStructures(const DataStructures &other) = default;

  DataStructures(DataStructures &&other)                 = delete;
  DataStructures &operator=(const DataStructures &other) = delete;

  bool has(addr_t addr) const { return objs().find(addr) != objs().end(); }
  bool has(DS_ID id) const { return ids().find(id) != ids().end(); }

  const std::unordered_set<DS *> &get_ds(addr_t addr) const {
    auto found_it = objs().find(addr);
    if (found_it == objs().end()) {
      panic("Data structure not found with addr %lu", addr);
    }
    return found_it->second;
  }

  const std::unordered_map<addr_t, std::unordered_set<DS *>> &get_data_per_obj() const { return objs(); }
  const std::unordered_map<DS_ID, DS *> &get_data_per_id() const { return ids(); }

  template <class DS_T> const DS_T *get_single_ds(addr_t addr) const {
    const std::unordered_set<DS *> &ds = get_ds(addr);
    assert(ds.size() == 1 && "Expected exactly one DS");
    return dynamic_cast<const DS_T *>(*ds.begin());
  }

  const DS *get_ds_from_id(DS_ID id) const {
    auto it = ids().find(id);
    if (it == ids().end()) {
      for (const std::shared_ptr<DS> &ds : all()) {
        const std::vector<DS_ID> unwrapped_ids = ds->unwrap();
        if (std::find(unwrapped_ids.begin(), unwrapped_ids.end(), id) != unwrapped_ids.end()) {
          return ds.get();
        }
      }
      panic("Data structure %s not found", id.c_str());
    }
    return it->second;
  }

  void save(addr_t addr, std::unique_ptr<DS> ds) {
    if (std::find_if(impl->data.begin(), impl->data.end(), [&ds](const std::shared_ptr<DS> &d) { return d.get() == ds.get(); }) != impl->data.end()) {
      return;
    }

    Impl &m = impl.mutate();

    auto found_it = m.data_per_id.find(ds->id);

    if (found_it != m.data_per_id.end()) {
      DS *old = found_it->second;
      assert(old->id == ds->id && "Data structure ID mismatch");
      m.data.erase(std::remove_if(m.data.begin(), m.data.end(), [old](const std::shared_ptr<DS> &d) { return d.get() == old; }), m.data.end());
      m.data_per_id.erase(ds->id);
      m.data_per_obj[addr].erase(old);
    }

    m.data_per_obj[addr].insert(ds.get());
    m.data_per_id[ds->id] = ds.get();
    m.data.emplace_back(std::move(ds));
  }

  void debug() const {
    std::cerr << "************ Data Structures ************\n";
    for (const auto &[addr, dss] : objs()) {
      std::cerr << "@" << addr << ": [";
      for (const DS *ds : dss) {
        std::cerr << ds->id << ",";
      }
      std::cerr << "]\n";
    }
    for (const auto &[_, dss] : objs()) {
      for (const DS *ds : dss) {
        ds->debug();
      }
    }
    std::cerr << "*****************************************\n";
  }
};

} // namespace Tofino
} // namespace LibSynapse