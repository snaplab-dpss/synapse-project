#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

namespace {

FCFSCachedTable *build_fcfs_ct(const EP *ep, const BDDNode *node, DS_ID id, klee::ref<klee::Expr> key, u32 capacity, u32 cache_capacity) {
  const Context &ctx                 = ep->get_ctx();
  const TofinoContext *tofino_ctx    = ctx.get_target_ctx<TofinoContext>();
  const TNA &tna                     = tofino_ctx->get_tna();
  const tna_properties_t &properties = tna.tna_config.properties;

  const std::vector<klee::ref<klee::Expr>> keys = Register::partition_value(properties, key, ctx.get_expr_structs());

  std::vector<bits_t> keys_sizes;
  for (klee::ref<klee::Expr> k : keys) {
    keys_sizes.push_back(k->getWidth());
  }

  const u8 used_digests = tofino_ctx->get_tna().pipeline.get_used_digests() + 1;

  FCFSCachedTable *fcfs_ct = new FCFSCachedTable(properties, id, node->get_id(), cache_capacity, capacity, keys_sizes, used_digests);

  if (!tofino_ctx->can_place(ep, node, fcfs_ct)) {
    delete fcfs_ct;
    fcfs_ct = nullptr;
  }

  return fcfs_ct;
}

FCFSCachedTable *internal_get_fcfs_ct(const EP *ep, const BDDNode *node, addr_t obj) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();

  if (!tofino_ctx->get_data_structures().has(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(obj);

  assert(ds.size() == 1 && "Invalid number of DS");
  assert((*ds.begin())->type == DSType::FCFSCachedTable && "Invalid DS type");

  return dynamic_cast<FCFSCachedTable *>(*ds.begin());
}

FCFSCachedTable *reuse_fcfs_ct(const EP *ep, const BDDNode *node, addr_t obj, u32 cache_capacity) {
  FCFSCachedTable *fcfs_ct = internal_get_fcfs_ct(ep, node, obj);
  assert(fcfs_ct && "FCFS cached table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  assert(!fcfs_ct->has_table(node->get_id()));

  if (fcfs_ct->cache_capacity != cache_capacity) {
    return nullptr;
  }

  std::optional<DS_ID> added_table_id;
  if (!fcfs_ct->has_table(node->get_id())) {
    added_table_id = fcfs_ct->add_table(node->get_id());
  }

  if (!tofino_ctx->can_place(ep, node, fcfs_ct)) {
    if (added_table_id.has_value()) {
      fcfs_ct->remove_table(added_table_id.value());
    }
    fcfs_ct = nullptr;
  }

  return fcfs_ct;
}

} // namespace

DS_ID TofinoModuleFactory::build_fcfs_ct_id(addr_t obj) { return "fcfs_ct_" + std::to_string(obj); }

FCFSCachedTable *TofinoModuleFactory::build_or_reuse_fcfs_ct(const EP *ep, const BDDNode *node, addr_t obj, klee::ref<klee::Expr> key, u32 capacity,
                                                             u32 cache_capacity) {
  FCFSCachedTable *fcfs_ct = nullptr;

  const Context &ctx        = ep->get_ctx();
  const bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable);

  if (already_placed) {
    fcfs_ct = reuse_fcfs_ct(ep, node, obj, cache_capacity);
  } else {
    const DS_ID id = build_fcfs_ct_id(obj);
    fcfs_ct        = build_fcfs_ct(ep, node, id, key, capacity, cache_capacity);
  }

  return fcfs_ct;
}

bool TofinoModuleFactory::can_reuse_fcfs_ct(const EP *ep, const BDDNode *node, addr_t obj, u32 cache_capacity) {
  FCFSCachedTable *fcfs_ct = internal_get_fcfs_ct(ep, node, obj);
  assert(fcfs_ct && "FCFS cached table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  assert(!fcfs_ct->has_table(node->get_id()));

  if (fcfs_ct->cache_capacity != cache_capacity) {
    return false;
  }

  std::unique_ptr<FCFSCachedTable> clone = std::unique_ptr<FCFSCachedTable>(dynamic_cast<FCFSCachedTable *>(fcfs_ct->clone()));
  clone->add_table(node->get_id());

  return tofino_ctx->can_place(ep, node, clone.get());
}

bool TofinoModuleFactory::can_build_or_reuse_fcfs_ct(const EP *ep, const BDDNode *node, addr_t obj, klee::ref<klee::Expr> key, u32 capacity,
                                                     u32 cache_capacity) {
  const Context &ctx        = ep->get_ctx();
  const bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable);

  if (already_placed) {
    return can_reuse_fcfs_ct(ep, node, obj, cache_capacity);
  }

  const DS_ID id           = build_fcfs_ct_id(obj);
  FCFSCachedTable *fcfs_ct = build_fcfs_ct(ep, node, id, key, capacity, cache_capacity);

  if (!fcfs_ct) {
    return false;
  }

  delete fcfs_ct;
  return true;
}

FCFSCachedTable *TofinoModuleFactory::get_fcfs_ct(const EP *ep, const BDDNode *node, addr_t obj) { return internal_get_fcfs_ct(ep, node, obj); }

std::vector<u32> TofinoModuleFactory::enum_fcfs_ct_cache_capacities() {
  std::vector<u32> capacities;

  u32 cache_capacity = 8;
  while (cache_capacity <= FCFSCachedTable::MAX_CACHE_CAPACITY) {
    capacities.push_back(cache_capacity);
    cache_capacity *= 2;
  }

  return capacities;
}

hit_rate_t TofinoModuleFactory::get_fcfs_ct_cache_collision_probability(const Context &ctx, const BDDNode *map_put, klee::ref<klee::Expr> key,
                                                                        u32 cache_capacity) {
  const flow_stats_t flow_stats = ctx.get_profiler().get_flow_stats(map_put, key);
  const u32 mask                = cache_capacity - 1;
  assert_or_panic(flow_stats.crc32_hashes_per_mask.contains(mask), "Failed to find crc32 hash for mask %u", mask);
  const u64 total_flow_hashes = flow_stats.crc32_hashes_per_mask.at(mask);

  const hit_rate_t collision_probability = 1_hr - hit_rate_t(total_flow_hashes, flow_stats.flows);

  // std::cerr << "\n";
  // std::cerr << "Cache capacity: " << cache_capacity << "\n";
  // std::cerr << "Total flow hashes for mask " << mask << ": " << total_flow_hashes << "\n";
  // std::cerr << "Collision probability: " << collision_probability << "\n";
  // const u64 controller_flows             = flow_stats.flows * collision_probability.value;
  // std::cerr << "Expected controller flows: " << controller_flows << "\n";

  return collision_probability;
}

} // namespace Tofino
} // namespace LibSynapse