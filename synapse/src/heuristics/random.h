#pragma once

#include "heuristic.h"

class RandomCfg : public HeuristicCfg {
public:
  RandomCfg()
      : HeuristicCfg("Random", {
                                   BUILD_METRIC(RandomCfg, get_random, MAX),
                               }) {}

  RandomCfg &operator=(const RandomCfg &other) {
    assert(other.name == name);
    assert(other.metrics.size() == metrics.size());
    return *this;
  }

private:
  i64 get_random(const EP *ep) const;
};

using Random = Heuristic<RandomCfg>;
