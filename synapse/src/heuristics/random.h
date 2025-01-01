#pragma once

#include "heuristic.h"

class RandomCfg : public HeuristicCfg {
public:
  RandomCfg()
      : HeuristicCfg("Random", {
                                   BUILD_METRIC(RandomCfg, get_random, MAX),
                               }) {}

  RandomCfg &operator=(const RandomCfg &other) {
    ASSERT(other.name == name, "Mismatched names");
    ASSERT(other.metrics.size() == metrics.size(), "Mismatched metrics");
    return *this;
  }

private:
  i64 get_random(const EP *ep) const;
};
