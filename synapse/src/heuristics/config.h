#pragma once

#include <functional>
#include <vector>

#include "score.h"
#include "../execution_plan/execution_plan.h"
#include "../targets/module_generator.h"

#define BUILD_METRIC(cls, name, obj)                                           \
  { std::bind(&cls::name, this, std::placeholders::_1), Metric::Objective::obj }

class HeuristicCfg {
protected:
  struct Metric {
    std::function<i64(const EP *)> computer;
    enum class Objective { MIN, MAX } objective;
  };

public:
  const std::string name;
  const std::vector<Metric> metrics;

  HeuristicCfg(const std::string &_name, const std::vector<Metric> &_metrics)
      : name(_name), metrics(_metrics) {}

  Score score(const EP *e) const {
    size_t N = metrics.size();
    std::vector<i64> values(N);

    for (size_t i = 0; i < N; i++) {
      values[i] = (metrics[i].computer)(e);
      if (metrics[i].objective == Metric::Objective::MIN) {
        values[i] *= -1;
      }
    }

    return Score(values);
  }

  virtual bool operator()(const impl_t &i1, const impl_t &i2) const {
    return score(i1.result) > score(i2.result);
  }

  virtual bool mutates(const impl_t &impl) const { return false; }
};
