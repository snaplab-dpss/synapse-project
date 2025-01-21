#pragma once

#include <LibSynapse/Heuristics/Heuristic.h>

namespace LibSynapse {

class GalliumCfg : public HeuristicCfg {
public:
  GalliumCfg()
      : HeuristicCfg("Gallium", {
                                    BUILD_METRIC(GalliumCfg, get_bdd_progress, MAX),
                                }) {}

  GalliumCfg &operator=(const GalliumCfg &other) {
    assert(other.name == name && "Mismatched names");
    assert(other.metrics.size() == metrics.size() && "Mismatched metrics");
    return *this;
  }

private:
  i64 get_switch_progression_nodes(const EP *ep) const {
    i64 tofino_decisions = 0;

    const EPMeta &meta   = ep->get_meta();
    auto tofino_nodes_it = meta.steps_per_target.find(TargetType::Tofino);

    if (tofino_nodes_it != meta.steps_per_target.end()) {
      tofino_decisions += tofino_nodes_it->second;
    }

    return tofino_decisions;
  }

  i64 get_bdd_progress(const EP *ep) const {
    const EPMeta &meta = ep->get_meta();
    return meta.processed_nodes.size();
  }
};

} // namespace LibSynapse