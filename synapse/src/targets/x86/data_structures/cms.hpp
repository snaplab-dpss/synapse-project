#pragma once

#include "data_structure.hpp"

namespace synapse {
namespace x86 {

struct cms_t : ds_t {
  u64 index_range;

  cms_t(addr_t _addr, node_id_t _node_id, u64 _index_range)
      : ds_t(ds_type_t::CMS, _addr, _node_id), index_range(_index_range) {}
};

} // namespace x86
} // namespace synapse