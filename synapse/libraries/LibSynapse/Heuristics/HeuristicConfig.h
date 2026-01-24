#pragma once

#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Heuristics/Score.h>

#include <functional>
#include <vector>

#define BUILD_METRIC(cls, name, obj)                                                                                                                 \
  { std::bind(&cls::name, this, std::placeholders::_1), obj }

namespace LibSynapse {

struct heuristic_metadata_t {
  std::string name;
  std::string description;
};

class HeuristicCfg {
protected:
  enum class Objective { Min, Max };

  struct Metric {
    std::function<i64(const EP *)> computer;
    Objective objective;
  };

public:
  const std::string name;
  const std::vector<Metric> metrics;
  const size_t total_metrics;
  std::unordered_set<ep_id_t> forced_search_decisions;

  HeuristicCfg(const std::string &_name, const std::vector<Metric> &_metrics) : name(_name), metrics(_metrics), total_metrics(_metrics.size()) {}

  virtual ~HeuristicCfg() = default;

  Score score(const EP *ep) const {
    std::vector<i64> values(total_metrics + 1);

    values[0] = get_enforced_decisions(ep);

    for (size_t i = 0; i < total_metrics; i++) {
      values[i + 1] = (metrics[i].computer)(ep);

      switch (metrics[i].objective) {
      case Objective::Min: {
        values[i + 1] *= -1;
      } break;
      case Objective::Max: {
        // Do nothing
      } break;
      }
    }

    return Score(values);
  }

  virtual bool operator()(const EP *e1, const EP *e2) const { return score(e1) > score(e2); }
  virtual bool mutates(const EP *ep) const { return false; }
  virtual std::vector<heuristic_metadata_t> get_metadata(const EP *ep) const { return {}; }

  void set_forced_search_decisions(const std::vector<ep_id_t> &decisions) {
    forced_search_decisions = std::unordered_set<ep_id_t>(decisions.begin(), decisions.end());
  }

protected:
  static heuristic_metadata_t build_meta_tput_estimate(const EP *ep);
  static heuristic_metadata_t build_meta_tput_speculation(const EP *ep);

  i64 get_enforced_decisions(const EP *ep) const {
    u32 enforced_decisions = 0;

    if (forced_search_decisions.contains(ep->get_id())) {
      enforced_decisions++;
    }

    for (ep_id_t ancestor_id : ep->get_ancestors()) {
      if (forced_search_decisions.contains(ancestor_id)) {
        enforced_decisions++;
      }
    }

    return enforced_decisions;
  }
};

} // namespace LibSynapse