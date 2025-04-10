#pragma once

#include <LibSynapse/Heuristics/Heuristic.h>

namespace LibSynapse {

class DSPrefCfg : public HeuristicCfg {
public:
  DSPrefCfg()
      : HeuristicCfg("DSPref", {
                                   BUILD_METRIC(DSPrefCfg, get_bdd_progress, Objective::Max),
                                   BUILD_METRIC(DSPrefCfg, get_ds_score, Objective::Max),
                               }) {}

  DSPrefCfg &operator=(const DSPrefCfg &other) {
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

  i64 get_ds_score(const EP *ep) const {
    const std::unordered_map<DSImpl, int> ds_scores{
        {DSImpl::Tofino_HeavyHitterTable, 1},
    };

    i64 score = 0;

    for (const auto &[addr, ds] : ep->get_ctx().get_ds_impls()) {
      auto it = ds_scores.find(ds);
      if (it != ds_scores.end()) {
        score += it->second;
      }
    }

    return score;
  }
};

} // namespace LibSynapse