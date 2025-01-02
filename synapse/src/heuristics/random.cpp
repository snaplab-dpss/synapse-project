#include "random.h"
#include "../targets/module.h"
#include "../log.h"

namespace synapse {
i64 RandomCfg::get_random(const EP *ep) const {
  const EPMeta &meta = ep->get_meta();
  return meta.random_number;
}
} // namespace synapse