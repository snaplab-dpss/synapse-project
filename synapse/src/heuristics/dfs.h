#pragma once

#include "heuristic.h"

class DFSCfg : public HeuristicCfg {
public:
  DFSCfg()
      : HeuristicCfg("DFS", {
                                BUILD_METRIC(DFSCfg, get_depth, MAX),
                            }) {}

  DFSCfg &operator=(const DFSCfg &other) {
    assert(other.name == name);
    assert(other.metrics.size() == metrics.size());
    return *this;
  }

private:
  i64 get_depth(const EP *ep) const;
};

using DFS = Heuristic<DFSCfg>;
