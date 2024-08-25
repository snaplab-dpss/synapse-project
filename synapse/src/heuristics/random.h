#pragma once

#include "heuristic.h"
#include "score.h"

struct RandomComparator : public HeuristicCfg {
  RandomComparator() : HeuristicCfg("Random") {}

  Score get_score(const EP *ep) const override {
    Score score(ep, {
                        {ScoreCategory::Random, ScoreObjective::MAX},
                    });
    return score;
  }
};

using Random = Heuristic<RandomComparator>;
