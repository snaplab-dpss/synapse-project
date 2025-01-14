#include "ds_pref.hpp"
#include "../targets/module.hpp"
#include "../system.hpp"

namespace synapse {
i64 DSPrefCfg::get_bdd_progress(const EP *ep) const {
  const EPMeta &meta = ep->get_meta();
  return meta.processed_nodes.size();
}

i64 DSPrefCfg::get_ds_score(const EP *ep) const {
  std::unordered_map<DSImpl, int> ds_scores = {
      {DSImpl::Tofino_HeavyHitterTable, 1},
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
} // namespace synapse