#pragma once

#include <unordered_set>
#include <string>
#include <assert.h>

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

  DS(DSType _type, bool _primitive, DS_ID _id)
      : type(_type), primitive(_primitive), id(_id) {}

  virtual ~DS() {}
  virtual DS *clone() const = 0;
  virtual void debug() const = 0;

  virtual std::vector<std::unordered_set<const DS *>> get_internal() const {
    assert(primitive && "Only non primitive data structures have internals");
    return {};
  }
};

} // namespace tofino
