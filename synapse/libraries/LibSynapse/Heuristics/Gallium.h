#pragma once

#include <LibSynapse/Heuristics/Heuristic.h>

namespace LibSynapse {

class GalliumCfg : public HeuristicCfg {
public:
  GalliumCfg()
      : HeuristicCfg("Gallium", {
                                    BUILD_METRIC(GalliumCfg, get_bdd_progress, Objective::Max),
                                }) {}

  GalliumCfg &operator=(const GalliumCfg &other) {
    assert(other.name == name && "Mismatched names");
    assert(other.metrics.size() == metrics.size() && "Mismatched metrics");
    return *this;
  }

  virtual std::vector<heuristic_metadata_t> get_metadata(const EP *ep) const override {
    return {
        build_meta_tput_estimate(ep),
    };
  }

private:
  i64 get_bdd_progress(const EP *ep) const {
    const EPMeta &meta = ep->get_meta();
    return meta.processed_nodes.size();
  }
};

} // namespace LibSynapse