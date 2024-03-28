#pragma once

#include "heuristic.h"
#include "score.h"

namespace synapse {

struct BiggestComparator : public HeuristicConfiguration {
  Score get_score(const ExecutionPlan &ep) const override {
    Score score(ep, {
                        {Score::Category::NumberOfNodes, Score::MAX},
                    });
    return score;
  }

  bool terminate_on_first_solution() const override { return false; }
};

using Biggest = Heuristic<BiggestComparator>;
} // namespace synapse
