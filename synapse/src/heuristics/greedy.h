#pragma once

#include "heuristic.h"

class GreedyCfg : public HeuristicCfg {
public:
  GreedyCfg()
      : HeuristicCfg("Greedy",
                     {
                         BUILD_METRIC(GreedyCfg, get_bdd_progress, MAX),
                         BUILD_METRIC(GreedyCfg, get_tput, MAX),
                     }) {}

  GreedyCfg &operator=(const GreedyCfg &other) {
    ASSERT(other.name == name, "Mismatched names");
    ASSERT(other.metrics.size() == metrics.size(), "Mismatched metrics");
    return *this;
  }

private:
  i64 get_switch_progression_nodes(const EP *ep) const;
  i64 get_bdd_progress(const EP *ep) const;
  i64 get_tput(const EP *ep) const;
};

using Greedy = Heuristic<GreedyCfg>;
