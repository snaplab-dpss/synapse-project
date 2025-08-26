#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

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
                                                              klee::ref<klee::Expr> key, u32 capacity) {
  constexpr const u32 THRESHOLD{16383};

  const std::vector<klee::ref<klee::Expr>> constraints = node->get_ordered_branch_constraints();
  const flow_stats_t flow_stats                        = ctx.get_profiler().get_flow_stats(constraints, key);
  const bdd_profile_t *bdd_profile                     = ctx.get_profiler().get_bdd_profile();
  const hit_rate_t node_hr                             = ctx.get_profiler().get_hr(node);

  u64 top_k = 0;
  for (size_t k = 0; k <= capacity && k < flow_stats.pkts_per_flow.size(); k++) {
    top_k += flow_stats.pkts_per_flow[k];
  }

  assert(top_k <= flow_stats.pkts && "Invalid top_k");
  const hit_rate_t steady_state_hit_rate(top_k, flow_stats.pkts);

  const double rate = node_hr.value * ep->estimate_tput_pps();

  const fpm_t churn_top_k_flows          = bdd_profile->churn_top_k_flows(map, capacity);
  const double churn_top_k_flows_per_sec = churn_top_k_flows / 60.0;

  // const hit_rate_t hit_rate = steady_state_hit_rate - ((churn_top_k_flows_per_sec * THRESHOLD) / rate);

  const double time_to_insert_in_table_sec = 0.1;
  const int n                              = 1;
  u64 top_n                                = 0;
  for (size_t k = 0; k < n && k < flow_stats.pkts_per_flow.size(); k++) {
    top_n += flow_stats.pkts_per_flow[k];
  }
  const hit_rate_t missing_flow_hit_rate = hit_rate_t(top_n, flow_stats.pkts * n);
  // const hit_rate_t missing_flow_hit_rate = hit_rate_t(steady_state_hit_rate.value, capacity);
  const hit_rate_t hit_rate =
      steady_state_hit_rate - churn_top_k_flows_per_sec * ((THRESHOLD / rate) + (missing_flow_hit_rate * time_to_insert_in_table_sec));

  return hit_rate;
}

} // namespace Tofino
} // namespace LibSynapse