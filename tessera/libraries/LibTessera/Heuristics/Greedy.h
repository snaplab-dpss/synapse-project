#pragma once

#include <LibTessera/Heuristics/Heuristic.h>

namespace LibTessera {

class Greedy : public HeuristicCfg {
public:
  Greedy()
      : HeuristicCfg("Greedy", {
                                   BUILD_METRIC(Greedy, get_switch_nodes_besides_sending_to_controller, Objective::Max),
                                   BUILD_METRIC(Greedy, get_bdd_progress, Objective::Max),
                                   BUILD_METRIC(Greedy, get_tput, Objective::Max),
                               }) {}

  Greedy &operator=(const Greedy &other) {
    assert(other.name == name && "Mismatched names");
    assert(other.metrics.size() == metrics.size() && "Mismatched metrics");
    return *this;
  }

  virtual std::vector<heuristic_metadata_t> get_metadata(const EP *ep) const override {
    return {
        build_meta_tput_estimate(ep),
        build_meta_tput_speculation(ep),
    };
  }

private:
  i64 get_bdd_progress(const EP *ep) const {
    const EPMeta &meta = ep->get_meta();
    return meta.processed_nodes.size();
  }

  i64 get_tput(const EP *ep) const { return ep->estimate_tput_pps(); }

  size_t get_tofino_nodes(const EP *ep) const {
    const EPMeta &meta   = ep->get_meta();
    auto tofino_nodes_it = meta.steps_per_target.find(TargetType::Tofino);
    if (tofino_nodes_it != meta.steps_per_target.end()) {
      return tofino_nodes_it->second;
    }
    return 0;
  }

  size_t get_total_send_to_controller_nodes(const EP *ep) const {
    const EPMeta &meta                 = ep->get_meta();
    auto send_to_controller_counter_it = meta.modules_counter.find(ModuleType::Tofino_SendToController);
    if (send_to_controller_counter_it != meta.modules_counter.end()) {
      return send_to_controller_counter_it->second;
    }
    return 0;
  }

  i64 get_switch_nodes_besides_sending_to_controller(const EP *ep) const {
    const size_t tofino_decisions         = get_tofino_nodes(ep);
    const size_t send_to_controller_nodes = get_total_send_to_controller_nodes(ep);

    i64 final_count = tofino_decisions;
    if (send_to_controller_nodes > 0) {
      assert(tofino_decisions >= send_to_controller_nodes && "Negative count");
      final_count -= send_to_controller_nodes;
    }

    return final_count;
  }
};

} // namespace LibTessera