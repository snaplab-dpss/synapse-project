#pragma once

#include <LibBDD/BDD.h>

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace LibSynapse {
namespace x86 {

enum class ds_type_t {
  Map,
  Vector,
  Dchain,
  CMS,
  CHT,
};

struct ds_t {
  ds_type_t type;
  addr_t addr;
  LibBDD::node_id_t node_id;

  ds_t(ds_type_t _type, addr_t _addr, LibBDD::node_id_t _node_id) : type(_type), addr(_addr), node_id(_node_id) {}

  bool matches(ds_type_t _type, addr_t _addr) const {
    if (type != _type) {
      return false;
    }

    return matches(_addr);
  }

  bool matches(addr_t _addr) const {
    if (addr != _addr) {
      return false;
    }

    return true;
  }
};

} // namespace x86
} // namespace LibSynapse