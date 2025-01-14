#pragma once

#include "heuristic.hpp"

namespace synapse {
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

private:
  i64 get_random(const EP *ep) const;
};
} // namespace synapse