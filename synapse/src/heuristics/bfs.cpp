#include "bfs.hpp"
#include "../targets/module.hpp"
#include "../system.hpp"

namespace synapse {
i64 BFSCfg::get_depth(const EP *ep) const {
  const EPMeta &meta = ep->get_meta();
  return meta.depth;
}
} // namespace synapse