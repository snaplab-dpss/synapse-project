#include "greedy.h"
#include "../targets/module.h"
#include "../system.h"

namespace synapse {
i64 GreedyCfg::get_switch_progression_nodes(const EP *ep) const {
  i64 tofino_decisions = 0;

  const EPMeta &meta   = ep->get_meta();
  auto tofino_nodes_it = meta.steps_per_target.find(TargetType::Tofino);

  if (tofino_nodes_it != meta.steps_per_target.end()) {
    tofino_decisions += tofino_nodes_it->second;
  }

  return tofino_decisions;
}

i64 GreedyCfg::get_bdd_progress(const EP *ep) const {
  const EPMeta &meta = ep->get_meta();
  return meta.processed_nodes.size();
}

i64 GreedyCfg::get_tput(const EP *ep) const { return ep->estimate_tput_pps(); }
} // namespace synapse