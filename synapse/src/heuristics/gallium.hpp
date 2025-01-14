#pragma once

#include "heuristic.hpp"

namespace synapse {
class GalliumCfg : public HeuristicCfg {
public:
  GalliumCfg()
      : HeuristicCfg("Gallium", {
                                    BUILD_METRIC(GalliumCfg, get_bdd_progress, MAX),
                                }) {}

  GalliumCfg &operator=(const GalliumCfg &other) {
    assert(other.name == name && "Mismatched names");
    assert(other.metrics.size() == metrics.size() && "Mismatched metrics");
    return *this;
  }

private:
  i64 get_switch_progression_nodes(const EP *ep) const;
  i64 get_bdd_progress(const EP *ep) const;
};
} // namespace synapse