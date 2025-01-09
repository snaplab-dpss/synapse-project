#include "bfs.h"
#include "../targets/module.h"
#include "../system.h"

namespace synapse {
i64 BFSCfg::get_depth(const EP *ep) const {
  const EPMeta &meta = ep->get_meta();
  return meta.depth;
}
} // namespace synapse