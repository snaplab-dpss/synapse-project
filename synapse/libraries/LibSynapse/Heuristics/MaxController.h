#pragma once

#include <LibSynapse/Heuristics/Heuristic.h>

namespace LibSynapse {

class MaxController : public HeuristicCfg {
public:
  MaxController()
      : HeuristicCfg("MaxController", {
                                          BUILD_METRIC(MaxController, get_switch_nodes_besides_sending_to_controller, Objective::Min),
                                          BUILD_METRIC(MaxController, get_bdd_progress, Objective::Max),
                                      }) {}

  MaxController &operator=(const MaxController &other) {
    assert(other.name == name && "Mismatched names");
    assert(other.metrics.size() == metrics.size() && "Mismatched metrics");
    return *this;
  }

  virtual std::vector<heuristic_metadata_t> get_metadata(const EP *ep) const override {
    return {
        build_meta_tput_estimate(ep),
        {
            .name        = "TofinoNodes",
            .description = std::to_string(get_tofino_nodes(ep)),
        },
        {
            .name        = "SendToControllerNodes",
            .description = std::to_string(get_total_send_to_controller_nodes(ep)),
        },
        {
            .name        = "Diff",
            .description = std::to_string(get_switch_nodes_besides_sending_to_controller(ep)),
        },
    };
  }

private:
  size_t get_tofino_nodes(const EP *ep) const {
    const EPMeta &meta   = ep->get_meta();
    auto tofino_nodes_it = meta.steps_per_target.find(TargetType(TargetArchitecture::Tofino, ""));
    if (tofino_nodes_it != meta.steps_per_target.end()) {
      return tofino_nodes_it->second;
    }
    return 0;
  }

  size_t get_total_send_to_controller_nodes(const EP *ep) const {
    const EPMeta &meta                 = ep->get_meta();
    auto send_to_controller_counter_it = meta.modules_counter.find(ModuleType(ModuleCategory::Tofino_SendToController, ""));
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

  i64 get_bdd_progress(const EP *ep) const {
    const EPMeta &meta = ep->get_meta();
    return meta.processed_nodes.size();
  }
};

} // namespace LibSynapse