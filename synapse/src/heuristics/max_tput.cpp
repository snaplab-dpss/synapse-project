#include "max_tput.h"
#include "../targets/module.h"
#include "../log.h"

int64_t MaxTputCfg::get_tput_speculation(const EP *ep) const {
  return ep->speculate_throughput_pps();
}

int64_t MaxTputCfg::get_switch_progression_nodes(const EP *ep) const {
  int64_t tofino_decisions = 0;

  const EPMeta &meta = ep->get_meta();
  auto tofino_nodes_it = meta.steps_per_target.find(TargetType::Tofino);

  if (tofino_nodes_it != meta.steps_per_target.end()) {
    tofino_decisions += tofino_nodes_it->second;
  }

  return tofino_decisions;
}

int64_t MaxTputCfg::get_bdd_progress(const EP *ep) const {
  const EPMeta &meta = ep->get_meta();
  return meta.processed_nodes.size();
}