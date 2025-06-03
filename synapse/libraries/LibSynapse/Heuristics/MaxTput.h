#pragma once

#include <LibSynapse/Heuristics/Heuristic.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>

#include <unordered_map>

namespace LibSynapse {

class MaxTput : public HeuristicCfg {
private:
  struct decision_hasher_t {
    std::size_t operator()(const decision_t &decision) const {
      size_t hash = 0;

      hash ^= std::hash<LibBDD::node_id_t>{}(decision.node);
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
  MaxTput()
      : HeuristicCfg("MaxTput", {
                                    BUILD_METRIC(MaxTput, get_bdd_progress, Objective::Max),
                                    BUILD_METRIC(MaxTput, get_tput_speculation, Objective::Max),
                                    BUILD_METRIC(MaxTput, get_pipeline_usage, Objective::Min),
                                }) {}

  MaxTput &operator=(const MaxTput &other) {
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

  virtual std::vector<heuristic_metadata_t> get_metadata(const EP *ep) const override {
    return {
        build_meta_tput_estimate(ep),
        build_meta_tput_speculation(ep),
        heuristic_metadata_t{
            .name        = "Stages",
            .description = std::to_string(get_pipeline_usage(ep)),
        },
    };
  }

private:
  i64 get_bdd_progress(const EP *ep) const {
    const EPMeta &meta = ep->get_meta();
    return meta.processed_nodes.size();
  }

  i64 get_tput_speculation(const EP *ep) const { return ep->speculate_tput_pps(); }

  i64 get_pipeline_usage(const EP *ep) const {
    const Tofino::TNA &tna = ep->get_ctx().get_target_ctx<Tofino::TofinoContext>()->get_tna();
    return tna.get_simple_placer().get_used_stages();
  }
};

} // namespace LibSynapse