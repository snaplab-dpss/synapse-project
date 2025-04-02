#pragma once

#include <LibSynapse/Heuristics/Heuristic.h>

namespace LibSynapse {

class RandomCfg : public HeuristicCfg {
public:
  RandomCfg()
      : HeuristicCfg("Random", {
                                   BUILD_METRIC(RandomCfg, get_random, MAX),
                               }) {}

  RandomCfg &operator=(const RandomCfg &other) {
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
  i64 get_random(const EP *ep) const {
    const EPMeta &meta = ep->get_meta();
    return meta.random_number;
  }
};

} // namespace LibSynapse