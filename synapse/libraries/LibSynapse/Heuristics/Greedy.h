#pragma once

#include <LibSynapse/Heuristics/Heuristic.h>

namespace LibSynapse {

class GreedyCfg : public HeuristicCfg {
public:
  GreedyCfg()
      : HeuristicCfg("Greedy", {
                                   BUILD_METRIC(GreedyCfg, get_bdd_progress, Objective::Max),
                                   BUILD_METRIC(GreedyCfg, get_tput, Objective::Max),
                               }) {}

  GreedyCfg &operator=(const GreedyCfg &other) {
    assert(other.name == name && "Mismatched names");
    assert(other.metrics.size() == metrics.size() && "Mismatched metrics");
    return *this;
  }

  virtual std::vector<heuristic_metadata_t> get_metadata(const EP *ep) const override {
    return {
        build_meta_tput_estimate(ep),
    };
  }

private:
  i64 get_bdd_progress(const EP *ep) const {
    const EPMeta &meta = ep->get_meta();
    return meta.processed_nodes.size();
  }

  i64 get_tput(const EP *ep) const { return ep->estimate_tput_pps(); }
};

} // namespace LibSynapse