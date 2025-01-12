#include "max_tput.h"
#include "../targets/module.h"
#include "../system.h"

namespace synapse {
i64 MaxTputCfg::get_tput_speculation(const EP *ep) const { return ep->speculate_tput_pps(); }

i64 MaxTputCfg::get_switch_progression_nodes(const EP *ep) const {
  i64 tofino_decisions = 0;

  const EPMeta &meta   = ep->get_meta();
  auto tofino_nodes_it = meta.steps_per_target.find(TargetType::Tofino);

  if (tofino_nodes_it != meta.steps_per_target.end()) {
    tofino_decisions += tofino_nodes_it->second;
  }

  return tofino_decisions;
}

i64 MaxTputCfg::get_bdd_progress(const EP *ep) const {
  const EPMeta &meta = ep->get_meta();
  return meta.processed_nodes.size();
}
} // namespace synapse