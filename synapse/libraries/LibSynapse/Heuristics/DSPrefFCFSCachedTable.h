#pragma once

#include <LibSynapse/Heuristics/Heuristic.h>

namespace LibSynapse {

class DSPrefFCFSCachedTable : public HeuristicCfg {
public:
  DSPrefFCFSCachedTable()
      : HeuristicCfg("DSPrefFCFSCachedTable", {
                                                  BUILD_METRIC(DSPrefFCFSCachedTable, get_ds_score, Objective::Max),
                                                  BUILD_METRIC(DSPrefFCFSCachedTable, get_bdd_progress, Objective::Max),
                                              }) {}

  DSPrefFCFSCachedTable &operator=(const DSPrefFCFSCachedTable &other) {
    assert(other.name == name && "Mismatched names");
    assert(other.metrics.size() == metrics.size() && "Mismatched metrics");
    return *this;
  }

  virtual std::vector<heuristic_metadata_t> get_metadata(const EP *ep) const override {
    return {
        {
            .name        = "BDD Progress",
            .description = std::to_string(get_bdd_progress(ep)),
        },
        {
            .name        = "DS Score",
            .description = std::to_string(get_ds_score(ep)),
        },
        build_meta_tput_estimate(ep),
    };
  }

private:
  i64 get_bdd_progress(const EP *ep) const {
    const EPMeta &meta = ep->get_meta();
    return meta.processed_nodes.size();
  }

  i64 get_ds_score(const EP *ep) const {
    const std::unordered_map<DSImpl, int> ds_scores{
        {DSImpl::Tofino_MapTable, 0},       {DSImpl::Tofino_GuardedMapTable, 0},  {DSImpl::Tofino_VectorTable, 0},
        {DSImpl::Tofino_DchainTable, 0},    {DSImpl::Tofino_VectorRegister, 0},   {DSImpl::Tofino_FCFSCachedTable, 1},
        {DSImpl::Tofino_Meter, 0},          {DSImpl::Tofino_HeavyHitterTable, 0}, {DSImpl::Tofino_IntegerAllocator, 0},
        {DSImpl::Tofino_CountMinSketch, 0}, {DSImpl::Tofino_CuckooHashTable, 0},  {DSImpl::Tofino_LPM, 0},
    };

    i64 score = 0;

    for (const auto &[addr, ds] : ep->get_ctx().get_ds_impls()) {
      auto it = ds_scores.find(ds);
      if (it != ds_scores.end()) {
        score += it->second;
      }
    }

    return score;
  }

  i64 get_tput_speculation(const EP *ep) const { return ep->speculate_tput_pps(); }
};

} // namespace LibSynapse