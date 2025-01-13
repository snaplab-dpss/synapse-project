#pragma once

#include <unordered_map>

#include "heuristic.h"
#include "../targets/module_factory.h"

namespace synapse {
class MaxTputCfg : public HeuristicCfg {
private:
  struct decision_hasher_t {
    std::size_t operator()(const decision_t &decision) const {
      size_t hash = 0;

      hash ^= std::hash<node_id_t>{}(decision.node);
      hash ^= std::hash<ModuleType>{}(decision.module);

      for (const auto &[key, value] : decision.params) {
        hash ^= std::hash<std::string>{}(key);
        hash ^= std::hash<i32>{}(value);
      }

      return hash;
    }
  };

  std::unordered_map<decision_t, i64, decision_hasher_t> decisions_perf;

public:
  MaxTputCfg()
      : HeuristicCfg("MaxTput", {
                                    BUILD_METRIC(MaxTputCfg, get_bdd_progress, MAX),
                                    BUILD_METRIC(MaxTputCfg, get_tput_speculation, MAX),

                                    // Avoid desincentivising modules that expand the BDD.
                                    // BUILD_METRIC(MaxTputCfg, get_tput_speculation, MAX),
                                    // BUILD_METRIC(MaxTputCfg, get_switch_progression_nodes, MAX),
                                    // BUILD_METRIC(MaxTputCfg, get_bdd_progress, MAX),
                                }) {}

  MaxTputCfg &operator=(const MaxTputCfg &other) {
    assert(other.name == name && "Mismatched names");
    assert(other.metrics.size() == metrics.size() && "Mismatched metrics");
    decisions_perf = other.decisions_perf;
    return *this;
  }

  // virtual bool mutates(const impl_t &impl) const override {
  //   const EP *old_ep = impl.decision.ep;
  //   const EP *new_ep = impl.result;

  //   assert(new_ep);

  //   // Just an edge case for the first step on the search space.
  //   if (!old_ep) {
  //     return false;
  //   }

  //   pps_t old_spec = old_ep->speculate_tput_pps();
  //   pps_t new_spec = new_ep->speculate_tput_pps();

  //   return new_spec < old_spec;
  // }

private:
  i64 get_tput_speculation(const EP *ep) const;
  i64 get_switch_progression_nodes(const EP *ep) const;
  i64 get_bdd_progress(const EP *ep) const;
};
} // namespace synapse