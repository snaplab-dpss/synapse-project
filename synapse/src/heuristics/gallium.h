#pragma once

#include "heuristic.h"

class GalliumCfg : public HeuristicCfg {
public:
  GalliumCfg()
      : HeuristicCfg("Gallium", {
                                    BUILD_METRIC(GalliumCfg, get_bdd_progress, MAX),
                                }) {}

  GalliumCfg &operator=(const GalliumCfg &other) {
    ASSERT(other.name == name, "Mismatched names");
    ASSERT(other.metrics.size() == metrics.size(), "Mismatched metrics");
    return *this;
  }

private:
  i64 get_switch_progression_nodes(const EP *ep) const;
  i64 get_bdd_progress(const EP *ep) const;
};

using Gallium = Heuristic<GalliumCfg>;
