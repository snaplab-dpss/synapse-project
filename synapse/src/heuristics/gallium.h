#pragma once

#include "heuristic.h"

class GalliumCfg : public HeuristicCfg {
public:
  GalliumCfg()
      : HeuristicCfg(
            "Gallium",
            {
                BUILD_METRIC(GalliumCfg, get_switch_progression_nodes, MAX),

                // Some modules (e.g. SendToController) increase the
                // number of nodes in the BDD. This means that the
                // percentage of processed BDD nodes can decrease even
                // though the number of processed BDD nodes is
                // increasing, favoring other modules like Recirculate.
                BUILD_METRIC(GalliumCfg, get_bdd_progress, MAX),
            }) {}

  GalliumCfg &operator=(const GalliumCfg &other) {
    assert(other.name == name);
    assert(other.metrics.size() == metrics.size());
    return *this;
  }

private:
  i64 get_switch_progression_nodes(const EP *ep) const;
  i64 get_bdd_progress(const EP *ep) const;
};

using Gallium = Heuristic<GalliumCfg>;
