#pragma once

#include "heuristic.h"
#include "score.h"

namespace synapse {

struct MostCompactComparator : public HeuristicConfiguration {
  Score get_score(const ExecutionPlan &ep) const override {
    Score score(ep, {
                        {Score::Category::NumberOfNodes, Score::MIN},
                    });
    return score;
  }

  bool terminate_on_first_solution() const override { return false; }
};

using MostCompact = Heuristic<MostCompactComparator>;
} // namespace synapse
