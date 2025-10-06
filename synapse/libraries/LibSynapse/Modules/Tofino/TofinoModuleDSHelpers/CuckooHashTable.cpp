#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

namespace {

CuckooHashTable *build_cuckoo_hash_table(const EP *ep, const BDDNode *node, const TargetType type, addr_t obj, klee::ref<klee::Expr> key,
                                         u32 capacity) {
  const DS_ID id = "cuckoo_hash_table_" + std::to_string(obj);

  const TofinoContext *tofino_ctx    = ep->get_ctx().get_target_ctx<TofinoContext>(type);
  const tna_properties_t &properties = tofino_ctx->get_tna().tna_config.properties;

  const bits_t key_size = key->getWidth();
  if (key_size != CuckooHashTable::SUPPORTED_KEY_SIZE) {
    return nullptr;
  }

  CuckooHashTable *cuckoo_hash_table = new CuckooHashTable(properties, id, node->get_id(), capacity);

  if (!tofino_ctx->can_place(ep, node, cuckoo_hash_table)) {
    delete cuckoo_hash_table;
    return nullptr;
  }

  return cuckoo_hash_table;
}

CuckooHashTable *reuse_cuckoo_hash_table(const EP *ep, const BDDNode *node, const TargetType type, addr_t obj) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>(type);

  if (!tofino_ctx->get_data_structures().has(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(obj);

  assert(ds.size() == 1 && "Invalid number of DS");
  assert((*ds.begin())->type == DSType::CuckooHashTable && "Invalid DS type");

  CuckooHashTable *cuckoo_hash_table = dynamic_cast<CuckooHashTable *>(*ds.begin());

  if (!tofino_ctx->can_place(ep, node, cuckoo_hash_table)) {
    cuckoo_hash_table = nullptr;
  }

  return cuckoo_hash_table;
}

} // namespace

CuckooHashTable *TofinoModuleFactory::build_or_reuse_cuckoo_hash_table(const EP *ep, const BDDNode *node, const TargetType type, addr_t obj,
                                                                       klee::ref<klee::Expr> key, u32 capacity) {
  CuckooHashTable *cuckoo_hash_table = nullptr;

  if (ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_CuckooHashTable)) {
    cuckoo_hash_table = reuse_cuckoo_hash_table(ep, node, type, obj);
  } else {
    cuckoo_hash_table = build_cuckoo_hash_table(ep, node, type, obj, key, capacity);
  }

  return cuckoo_hash_table;
}

bool TofinoModuleFactory::can_build_or_reuse_cuckoo_hash_table(const EP *ep, const BDDNode *node, const TargetType type, addr_t obj,
                                                               klee::ref<klee::Expr> key, u32 capacity) {
  CuckooHashTable *cuckoo_hash_table = nullptr;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_CuckooHashTable);

  if (already_placed) {
    const TofinoContext *tofino_ctx    = ctx.get_target_ctx<TofinoContext>(type);
    const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(obj);

    assert(ds.size() == 1 && "Invalid number of DS");
    assert((*ds.begin())->type == DSType::CuckooHashTable && "Invalid DS type");

    cuckoo_hash_table = dynamic_cast<CuckooHashTable *>(*ds.begin());

    if (!tofino_ctx->can_place(ep, node, cuckoo_hash_table)) {
      cuckoo_hash_table = nullptr;
      return false;
    }

    return true;
  }

  cuckoo_hash_table = build_cuckoo_hash_table(ep, node, type, obj, key, capacity);

  if (!cuckoo_hash_table) {
    return false;
  }

  delete cuckoo_hash_table;
  return true;
}

hit_rate_t TofinoModuleFactory::get_cuckoo_hash_table_hit_success_rate(const EP *ep, const Context &ctx, const BDDNode *node, addr_t map,
                                                                       klee::ref<klee::Expr> key, u32 capacity) {
  constexpr const u32 THRESHOLD{16383};

  const flow_stats_t flow_stats    = ctx.get_profiler().get_flow_stats(node, key);
  const bdd_profile_t *bdd_profile = ctx.get_profiler().get_bdd_profile();
  const hit_rate_t node_hr         = ctx.get_profiler().get_hr(node);

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
