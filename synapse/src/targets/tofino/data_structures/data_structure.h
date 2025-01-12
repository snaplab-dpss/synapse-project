#pragma once

#include <unordered_set>
#include <string>
#include <vector>

#include "../../../system.h"

namespace synapse {
namespace tofino {

typedef std::string DS_ID;

enum class DSType {
  TABLE,
  REGISTER,
  METER,
  HASH,
  FCFS_CACHED_TABLE,
  HH_TABLE,
  COUNT_MIN_SKETCH,
};

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

} // namespace tofino
} // namespace synapse