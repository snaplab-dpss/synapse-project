#pragma once

#include "heuristic.h"

class BFSCfg : public HeuristicCfg {
public:
  BFSCfg()
      : HeuristicCfg("BFS", {
                                BUILD_METRIC(BFSCfg, get_depth, MIN),
                            }) {}

  BFSCfg &operator=(const BFSCfg &other) {
    ASSERT(other.name == name, "Mismatched names");
    ASSERT(other.metrics.size() == metrics.size(), "Mismatched metrics");
    return *this;
  }

private:
  i64 get_depth(const EP *ep) const;
};

using BFS = Heuristic<BFSCfg>;
