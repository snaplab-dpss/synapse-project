#pragma once

#include <LibSynapse/Heuristics/Heuristic.h>

namespace LibSynapse {

class BFSCfg : public HeuristicCfg {
public:
  BFSCfg()
      : HeuristicCfg("BFS", {
                                BUILD_METRIC(BFSCfg, get_depth, MIN),
                            }) {}

  BFSCfg &operator=(const BFSCfg &other) {
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