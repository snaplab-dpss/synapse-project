#pragma once

#include "heuristic.h"

class BFSCfg : public HeuristicCfg {
public:
  BFSCfg()
      : HeuristicCfg("BFS", {
                                BUILD_METRIC(BFSCfg, get_depth, MIN),
                            }) {}

  BFSCfg &operator=(const BFSCfg &other) {
    assert(other.name == name);
    assert(other.metrics.size() == metrics.size());
    return *this;
  }

private:
  i64 get_depth(const EP *ep) const;
};

using BFS = Heuristic<BFSCfg>;
