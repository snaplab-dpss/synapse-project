#pragma once

#include "heuristic.hpp"

namespace synapse {
class DSPrefCfg : public HeuristicCfg {
public:
  DSPrefCfg()
      : HeuristicCfg("DSPref", {
                                   BUILD_METRIC(DSPrefCfg, get_bdd_progress, MAX),
                                   BUILD_METRIC(DSPrefCfg, get_ds_score, MAX),
                               }) {}

  DSPrefCfg &operator=(const DSPrefCfg &other) {
    assert(other.name == name && "Mismatched names");
    assert(other.metrics.size() == metrics.size() && "Mismatched metrics");
    return *this;
  }

private:
  i64 get_bdd_progress(const EP *ep) const;
  i64 get_ds_score(const EP *ep) const;
};
} // namespace synapse