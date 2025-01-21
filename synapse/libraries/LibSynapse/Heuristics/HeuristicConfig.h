#pragma once

#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Heuristics/Score.h>

#include <functional>
#include <vector>

#define BUILD_METRIC(cls, name, obj)                                                                                                       \
  { std::bind(&cls::name, this, std::placeholders::_1), Metric::Objective::obj }

namespace LibSynapse {

class HeuristicCfg {
protected:
  struct Metric {
    std::function<i64(const EP *)> computer;
    enum class Objective { MIN, MAX } objective;
  };

public:
  const std::string name;
  const std::vector<Metric> metrics;

  HeuristicCfg(const std::string &_name, const std::vector<Metric> &_metrics) : name(_name), metrics(_metrics) {}

  virtual ~HeuristicCfg() = default;

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

  virtual bool operator()(const EP *e1, const EP *e2) const { return score(e1) > score(e2); }
  virtual bool mutates(const EP *ep) const { return false; }
};

} // namespace LibSynapse