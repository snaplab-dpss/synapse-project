#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>
#include <numbers>

namespace LibSynapse {
namespace Tofino {

namespace {

HHTable *build_hh_table(const EP *ep, const BDDNode *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys, u32 capacity, u32 cms_width,
                        u32 cms_height) {
  const DS_ID id = "hh_table_" + std::to_string(obj);

  const TofinoContext *tofino_ctx    = ep->get_ctx().get_target_ctx<TofinoContext>();
  const tna_properties_t &properties = tofino_ctx->get_tna().tna_config.properties;

  std::vector<bits_t> keys_sizes;
  for (klee::ref<klee::Expr> key : keys) {
    keys_sizes.push_back(key->getWidth());
  }

  const u8 used_digests = tofino_ctx->get_tna().pipeline.get_used_stages() + 1;
  HHTable *hh_table     = new HHTable(properties, id, node->get_id(), capacity, keys_sizes, cms_width, cms_height, used_digests);

  if (!tofino_ctx->can_place(ep, node, hh_table)) {
    delete hh_table;
    hh_table = nullptr;
  }

  return hh_table;
}

HHTable *reuse_hh_table(const EP *ep, const BDDNode *node, addr_t obj) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();

  if (!tofino_ctx->get_data_structures().has(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(obj);

  assert(ds.size() == 1 && "Invalid number of DS");
  assert((*ds.begin())->type == DSType::HHTable && "Invalid DS type");

  HHTable *hh_table = dynamic_cast<HHTable *>(*ds.begin());

  if (!tofino_ctx->can_place(ep, node, hh_table)) {
    hh_table = nullptr;
  }

  return hh_table;
}

} // namespace

HHTable *TofinoModuleFactory::build_or_reuse_hh_table(const EP *ep, const BDDNode *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys,
                                                      u32 capacity, u32 cms_width, u32 cms_height) {
  HHTable *hh_table = nullptr;

  if (ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    hh_table = reuse_hh_table(ep, node, obj);
  } else {
    hh_table = build_hh_table(ep, node, obj, keys, capacity, cms_width, cms_height);
  }

  return hh_table;
}

bool TofinoModuleFactory::can_build_or_reuse_hh_table(const EP *ep, const BDDNode *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys,
                                                      u32 capacity, u32 cms_width, u32 cms_height) {
  HHTable *hh_table = nullptr;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable);

  if (already_placed) {
    const TofinoContext *tofino_ctx    = ctx.get_target_ctx<TofinoContext>();
    const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(obj);

    assert(ds.size() == 1 && "Invalid number of DS");
    assert((*ds.begin())->type == DSType::HHTable && "Invalid DS type");

    hh_table = dynamic_cast<HHTable *>(*ds.begin());

    if (!tofino_ctx->can_place(ep, node, hh_table)) {
      hh_table = nullptr;
      return false;
    }

    if (hh_table->cms_width != cms_width || hh_table->cms_height != cms_height) {
      return false;
    }

    return true;
  }

  hh_table = build_hh_table(ep, node, obj, keys, capacity, cms_width, cms_height);

  if (!hh_table) {
    return false;
  }

  delete hh_table;
  return true;
}

hit_rate_t TofinoModuleFactory::get_hh_table_hit_success_rate(const EP *ep, const Context &ctx, const BDDNode *node, addr_t map,
                                                              klee::ref<klee::Expr> key, u32 capacity, u32 cms_width) {
  constexpr const u32 THRESHOLD{16383};

  const flow_stats_t flow_stats    = ctx.get_profiler().get_flow_stats(node, key);
  const bdd_profile_t *bdd_profile = ctx.get_profiler().get_bdd_profile();
  const hit_rate_t node_hr         = ctx.get_profiler().get_hr(node);

  const double rate                      = node_hr.value * ep->estimate_tput_pps();
  const fpm_t churn_top_k_flows          = bdd_profile->churn_top_k_flows(map, capacity);
  const double churn_top_k_flows_per_sec = churn_top_k_flows / 60.0;

  auto find_bottom_k_affected = [capacity, &flow_stats](u64 threshold) {
    assert(capacity < flow_stats.pkts_per_flow.size() && "Invalid capacity");
    u32 k = 0;
    for (size_t i = 0; i < capacity; i++) {
      if (flow_stats.pkts_per_flow[i] <= threshold) {
        k = capacity - i;
        break;
      }
    }
    return k;
  };

  const double epsilon = std::numbers::e / cms_width;

  u32 bottom_k_affected = 0;
  u32 spillover         = 0;

  while (spillover + capacity + 1 < flow_stats.pkts_per_flow.size()) {
    spillover++;
    const u64 upper_bound = (2 * flow_stats.pkts_per_flow.at(capacity + spillover) + epsilon * flow_stats.pkts) / 2;
    bottom_k_affected     = find_bottom_k_affected(upper_bound);
    if (bottom_k_affected == 0) {
      break;
    }

    // std::cerr << "\n";
    // std::cerr << "spillover:         " << spillover << "\n";
    // std::cerr << "pkts:              " << flow_stats.pkts_per_flow.at(capacity + spillover) << "\n";
    // std::cerr << "epsilon:           " << epsilon << "\n";
    // std::cerr << "upper_bound:       " << upper_bound << "\n";
    // std::cerr << "bottom_k_affected: " << bottom_k_affected << "\n";
    // dbg_pause();
  }

  const hit_rate_t new_hr                = flow_stats.calculate_top_k_hit_rate(capacity);
  const hit_rate_t unaffected_hr         = flow_stats.calculate_top_k_hit_rate(capacity - bottom_k_affected);
  const hit_rate_t affected_hr           = new_hr - unaffected_hr;
  const hit_rate_t contending_hr         = flow_stats.calculate_hit_rate_between(capacity + 1, capacity + spillover);
  const hit_rate_t new_affected_hr       = (affected_hr + contending_hr) / 2;
  const hit_rate_t steady_state_hit_rate = std::min(new_hr, unaffected_hr + new_affected_hr);

  // std::cerr << "\n";
  // std::cerr << "w=" << w << " epsilon=" << epsilon << " spillover=" << spillover << " bottom_k_affected=" << bottom_k_affected << "\n";
  // std::cerr << "new_hr                " << new_hr << "\n";
  // std::cerr << "unaffected_hr         " << unaffected_hr << "\n";
  // std::cerr << "affected_hr           " << affected_hr << "\n";
  // std::cerr << "contending_hr         " << contending_hr << "\n";
  // std::cerr << "new_affected_hr       " << new_affected_hr << "\n";
  // std::cerr << "steady_state_hit_rate " << steady_state_hit_rate << "\n";
  // dbg_pause();

  const double time_to_insert_in_table_sec = 1;
  const int n                              = 1;
  u64 top_n                                = 0;
  for (size_t k = 0; k < n && k < flow_stats.pkts_per_flow.size(); k++) {
    top_n += flow_stats.pkts_per_flow[k];
  }
  const hit_rate_t missing_flow_hit_rate = hit_rate_t(top_n, flow_stats.pkts * n);
  const hit_rate_t hit_rate =
      steady_state_hit_rate - churn_top_k_flows_per_sec * ((THRESHOLD / rate) + (missing_flow_hit_rate * time_to_insert_in_table_sec));

  return hit_rate;
}

} // namespace Tofino
} // namespace LibSynapse