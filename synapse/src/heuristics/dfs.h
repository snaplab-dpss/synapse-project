#pragma once

#include "heuristic.h"

namespace synapse {
class DFSCfg : public HeuristicCfg {
public:
  DFSCfg()
      : HeuristicCfg("DFS", {
                                BUILD_METRIC(DFSCfg, get_depth, MAX),
                            }) {}

  DFSCfg &operator=(const DFSCfg &other) {
    SYNAPSE_ASSERT(other.name == name, "Mismatched names");
    SYNAPSE_ASSERT(other.metrics.size() == metrics.size(), "Mismatched metrics");
    return *this;
  }

private:
  i64 get_depth(const EP *ep) const;
};
} // namespace synapse