#pragma once

#include <unordered_set>
#include <string>
#include <vector>
#include <cassert>

namespace LibSynapse {
namespace Tofino {

using DS_ID = std::string;

enum class DSType {
  // ========================
  // Primitive types
  // ========================

  Table,
  Register,
  Meter,
  Hash,
  Digest,

  // ========================
  // Compositional types
  // ========================

  MapTable,
  GuardedMapTable,
  VectorTable,
  DchainTable,
  VectorRegister,
  FCFSCachedTable,
  HHTable,
  CountMinSketch,
  LPM,
};

inline std::string ds_type_to_string(DSType type) {
  switch (type) {
  case DSType::Table:
    return "Table";
  case DSType::Register:
    return "Register";
  case DSType::Meter:
    return "Meter";
  case DSType::Hash:
    return "Hash";
  case DSType::Digest:
    return "Digest";
  case DSType::MapTable:
    return "MapTable";
  case DSType::GuardedMapTable:
    return "GuardedMapTable";
  case DSType::VectorTable:
    return "VectorTable";
  case DSType::DchainTable:
    return "DchainTable";
  case DSType::VectorRegister:
    return "VectorRegister";
  case DSType::FCFSCachedTable:
    return "FCFSCachedTable";
  case DSType::HHTable:
    return "HHTable";
  case DSType::CountMinSketch:
    return "CountMinSketch";
  case DSType::LPM:
    return "LPM";
  }
}

struct DS {
  DSType type;
  bool primitive;
  DS_ID id;

  DS(DSType _type, bool _primitive, DS_ID _id) : type(_type), primitive(_primitive), id(_id) {}

  virtual ~DS() {}
  virtual DS *clone() const  = 0;
  virtual void debug() const = 0;

  virtual std::vector<std::unordered_set<const DS *>> get_internal() const {
    assert(primitive && "Only non primitive data structures have internals");
    return {};
  }

  std::vector<std::unordered_set<const DS *>> get_internal_primitive() const {
    std::vector<std::unordered_set<const DS *>> primitives;

    for (const auto &data_structures : get_internal()) {
      primitives.emplace_back();

      std::vector<std::unordered_set<const DS *>> pending;

      for (const DS *ds : data_structures) {
        if (ds->primitive) {
          primitives.back().insert(ds);
          continue;
        }

        size_t i = 0;
        for (const auto &pds : ds->get_internal_primitive()) {
          if (pending.size() <= i) {
            pending.emplace_back();
            assert(pending.size() > i && "Invalid pending size");
          }

          pending[i].insert(pds.begin(), pds.end());
          i++;
        }
      }

      for (size_t i = 0; i < pending.size(); i++) {
        if (i != 0) {
          primitives.emplace_back();
        }

        primitives.back().insert(pending[i].begin(), pending[i].end());
      }
    }

    return primitives;
  }
};

} // namespace Tofino
} // namespace LibSynapse