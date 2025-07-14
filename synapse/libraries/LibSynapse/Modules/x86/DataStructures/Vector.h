#pragma once

#include <LibSynapse/Modules/x86/DataStructures/DataStructure.h>

namespace LibSynapse {
namespace x86 {

using LibBDD::bdd_node_id_t;

struct vector_t : ds_t {
  u64 index_range;

  vector_t(addr_t _addr, bdd_node_id_t _node_id, u64 _index_range) : ds_t(ds_type_t::Vector, _addr, _node_id), index_range(_index_range) {}
};

} // namespace x86
} // namespace LibSynapse