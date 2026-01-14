#pragma once

#include <LibSynapse/Heuristics/Heuristic.h>

namespace LibSynapse {

class Gallium : public HeuristicCfg {
public:
  Gallium()
      : HeuristicCfg("Gallium", {
                                    // Gallium only handles simple data structure implementations
                                    BUILD_METRIC(Gallium, get_incompatible_data_structures, Objective::Min),

                                    // Even for some compatible data structures, Gallium has limitations in terms of operations supported
                                    BUILD_METRIC(Gallium, get_incompatible_modules, Objective::Min),

                                    // No reordering allowed in Gallium
                                    BUILD_METRIC(Gallium, get_reordering_operations, Objective::Min),

                                    // Gallium tries to maximize switch nodes
                                    BUILD_METRIC(Gallium, get_switch_nodes_besides_sending_to_controller, Objective::Max),

                                    BUILD_METRIC(Gallium, get_bdd_progress, Objective::Max),
                                }) {}

  Gallium &operator=(const Gallium &other) {
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

  i64 get_recirculations(const EP *ep) const {
    const EPMeta &meta = ep->get_meta();
    auto it            = meta.modules_counter.find(ModuleType::Tofino_Recirculate);
    if (it == meta.modules_counter.end()) {
      return 0;
    }
    return it->second;
  }

  i64 get_incompatible_data_structures(const EP *ep) const {
    i64 count = 0;

    for (const auto &[addr, ds] : ep->get_ctx().get_ds_impls()) {
      switch (ds) {
        // ========================================
        // Tofino Gallium Compatible
        // ========================================

      case DSImpl::Tofino_MapTable:
      case DSImpl::Tofino_VectorTable:
      case DSImpl::Tofino_DchainTable:
      case DSImpl::Tofino_VectorRegister:
      case DSImpl::Tofino_CountMinSketch:
      case DSImpl::Tofino_BloomFilter:
        break;

        // ========================================
        // Tofino Gallium Inompatible
        // ========================================

      case DSImpl::Tofino_GuardedMapTable:
      case DSImpl::Tofino_FCFSCachedTable:
      case DSImpl::Tofino_Meter:
      case DSImpl::Tofino_HeavyHitterTable:
      case DSImpl::Tofino_IntegerAllocator:
      case DSImpl::Tofino_CuckooHashTable:
      case DSImpl::Tofino_LPM:
        count++;
        break;

        // ========================================
        // Controller
        // ========================================

      case DSImpl::Controller_Map:
      case DSImpl::Controller_Vector:
      case DSImpl::Controller_DoubleChain:
      case DSImpl::Controller_ConsistentHashTable:
      case DSImpl::Controller_CountMinSketch:
      case DSImpl::Controller_BloomFilter:
      case DSImpl::Controller_TokenBucket:
        break;

        // ========================================
        // x86
        // ========================================

      case DSImpl::x86_Map:
      case DSImpl::x86_Vector:
      case DSImpl::x86_DoubleChain:
      case DSImpl::x86_ConsistentHashTable:
      case DSImpl::x86_CountMinSketch:
      case DSImpl::x86_TokenBucket:
        break;
      }
    }

    return count;
  }

  i64 get_incompatible_modules(const EP *ep) const {
    const std::unordered_set<ModuleType> incompatible_modules = {
        ModuleType::Tofino_Recirculate,
        ModuleType::Tofino_VectorRegisterConditionalUpdate,
        ModuleType::Tofino_BloomFilterQueryAndSet,
    };

    i64 count = 0;
    for (const ModuleType &module_type : incompatible_modules) {
      if (ep->get_meta().modules_counter.contains(module_type)) {
        count += 1;
      }
    }

    return count;
  }

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

  i64 get_reordering_operations(const EP *ep) const {
    const EPMeta &meta = ep->get_meta();
    return meta.reordered_nodes;
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

} // namespace LibSynapse