#pragma once

#include "heuristic.h"

namespace synapse {
class BFSCfg : public HeuristicCfg {
public:
  BFSCfg()
      : HeuristicCfg("BFS", {
                                BUILD_METRIC(BFSCfg, get_depth, MIN),
                            }) {}

  BFSCfg &operator=(const BFSCfg &other) {
    SYNAPSE_ASSERT(other.name == name, "Mismatched names");
    SYNAPSE_ASSERT(other.metrics.size() == metrics.size(), "Mismatched metrics");
    return *this;
  }

private:
  i64 get_depth(const EP *ep) const;
};
} // namespace synapse