#pragma once

#include <LibSynapse/Heuristics/Heuristic.h>

namespace LibSynapse {

class DFSCfg : public HeuristicCfg {
public:
  DFSCfg()
      : HeuristicCfg("DFS", {
                                BUILD_METRIC(DFSCfg, get_depth, MAX),
                            }) {}

  DFSCfg &operator=(const DFSCfg &other) {
    assert(other.name == name && "Mismatched names");
    assert(other.metrics.size() == metrics.size() && "Mismatched metrics");
    return *this;
  }

private:
  i64 get_depth(const EP *ep) const {
    const EPMeta &meta = ep->get_meta();
    return meta.depth;
  }
};

} // namespace LibSynapse