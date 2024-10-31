#include "tofino_module.h"

namespace tofino {

std::unordered_set<DS *> TofinoModuleGenerator::build_vector_registers(
    const EP *ep, const Node *node, const vector_register_data_t &data,
    std::unordered_set<DS_ID> &rids, std::unordered_set<DS_ID> &deps) const {
  std::unordered_set<DS *> regs;

  const TNA &tna = get_tna(ep);
  const TNAProperties &properties = tna.get_properties();

  std::vector<klee::ref<klee::Expr>> partitions =
      Register::partition_value(properties, data.value);

  for (klee::ref<klee::Expr> partition : partitions) {
    DS_ID rid = "vector_" + std::to_string(node->get_id()) + "_" +
                std::to_string(rids.size());
    bits_t partition_size = partition->getWidth();
    Register *reg =
        new Register(properties, rid, data.num_entries, data.index->getWidth(),
                     partition_size, data.actions);
    regs.insert(reg);
    rids.insert(rid);
  }

  const TofinoContext *tofino_ctx = get_tofino_ctx(ep);
  deps = tofino_ctx->get_stateful_deps(ep, node);

  if (!tofino_ctx->check_many_placements(ep, {regs}, deps)) {
    for (DS *reg : regs) {
      delete reg;
    }
    regs.clear();
  }

  return regs;
}

std::unordered_set<DS *> TofinoModuleGenerator::get_vector_registers(
    const EP *ep, const Node *node, const vector_register_data_t &data,
    std::unordered_set<DS_ID> &rids, std::unordered_set<DS_ID> &deps) const {
  std::unordered_set<DS *> regs;

  const TofinoContext *tofino_ctx = get_tofino_ctx(ep);

  if (!tofino_ctx->has_ds(data.obj)) {
    return regs;
  }

  regs = tofino_ctx->get_ds(data.obj);
  assert(regs.size());

  for (DS *reg : regs) {
    assert(reg->type == DSType::REGISTER);
    rids.insert(reg->id);
  }

  deps = tofino_ctx->get_stateful_deps(ep, node);

  if (!tofino_ctx->check_many_placements(ep, {regs}, deps)) {
    regs.clear();
  }

  return regs;
}

std::unordered_set<DS *> TofinoModuleGenerator::get_or_build_vector_registers(
    const EP *ep, const Node *node, const vector_register_data_t &data,
    bool &already_exists, std::unordered_set<DS_ID> &rids,
    std::unordered_set<DS_ID> &deps) const {
  std::unordered_set<DS *> regs;

  const Context &ctx = ep->get_ctx();
  bool regs_already_placed =
      ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorRegister);

  if (regs_already_placed) {
    regs = get_vector_registers(ep, node, data, rids, deps);
    already_exists = true;
  } else {
    regs = build_vector_registers(ep, node, data, rids, deps);
    already_exists = false;
  }

  return regs;
}

bool TofinoModuleGenerator::can_get_or_build_vector_registers(
    const EP *ep, const Node *node, const vector_register_data_t &data) const {
  std::unordered_set<DS_ID> rids;
  std::unordered_set<DS_ID> deps;

  const Context &ctx = ep->get_ctx();
  bool regs_already_placed =
      ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorRegister);

  if (regs_already_placed) {
    std::unordered_set<DS *> regs =
        get_vector_registers(ep, node, data, rids, deps);
    return !regs.empty();
  }

  std::unordered_set<DS *> regs =
      build_vector_registers(ep, node, data, rids, deps);
  bool success = !regs.empty();

  for (DS *reg : regs) {
    delete reg;
  }

  return success;
}

void TofinoModuleGenerator::place_vector_registers(
    EP *ep, const vector_register_data_t &data,
    const std::unordered_set<DS *> &regs,
    const std::unordered_set<DS_ID> &deps) const {
  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(ep);
  ep->get_mutable_ctx().save_ds_impl(data.obj, DSImpl::Tofino_VectorRegister);
  tofino_ctx->place_many(ep, data.obj, {regs}, deps);
}

static FCFSCachedTable *build_fcfs_cached_table(const EP *ep, const Node *node,
                                                klee::ref<klee::Expr> key,
                                                int num_entries,
                                                int cache_capacity) {
  const Context &ctx = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const TNA &tna = tofino_ctx->get_tna();
  const TNAProperties &properties = tna.get_properties();

  DS_ID id = "fcfs_cached_table_" + std::to_string(cache_capacity);

  std::vector<klee::ref<klee::Expr>> keys =
      Register::partition_value(properties, key);
  std::vector<bits_t> keys_sizes;
  for (klee::ref<klee::Expr> key : keys) {
    keys_sizes.push_back(key->getWidth());
  }

  FCFSCachedTable *cached_table = new FCFSCachedTable(
      properties, id, node->get_id(), cache_capacity, num_entries, keys_sizes);

  std::unordered_set<DS_ID> deps = tofino_ctx->get_stateful_deps(ep, node);
  if (!tofino_ctx->check_placement(ep, cached_table, deps)) {
    delete cached_table;
    cached_table = nullptr;
  }

  return cached_table;
}

static FCFSCachedTable *reuse_fcfs_cached_table(const EP *ep, const Node *node,
                                                addr_t obj,
                                                int cache_capacity) {
  const Context &ctx = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

  assert(ds.size() == 1);
  assert((*ds.begin())->type == DSType::FCFS_CACHED_TABLE);

  FCFSCachedTable *cached_table = static_cast<FCFSCachedTable *>(*ds.begin());

  if (cached_table->cache_capacity != cache_capacity) {
    return nullptr;
  }

  if (!cached_table->has_table(node->get_id())) {
    FCFSCachedTable *clone =
        static_cast<FCFSCachedTable *>(cached_table->clone());
    clone->add_table(node->get_id());
    cached_table = clone;
  }

  std::unordered_set<DS_ID> deps = tofino_ctx->get_stateful_deps(ep, node);
  if (!tofino_ctx->check_placement(ep, cached_table, deps)) {
    delete cached_table;
    cached_table = nullptr;
  }

  return cached_table;
}

FCFSCachedTable *TofinoModuleGenerator::build_or_reuse_fcfs_cached_table(
    const EP *ep, const Node *node, addr_t obj, klee::ref<klee::Expr> key,
    int num_entries, int cache_capacity) const {
  FCFSCachedTable *cached_table = nullptr;

  const Context &ctx = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable);

  if (already_placed) {
    cached_table = reuse_fcfs_cached_table(ep, node, obj, cache_capacity);
  } else {
    cached_table =
        build_fcfs_cached_table(ep, node, key, num_entries, cache_capacity);
  }

  return cached_table;
}

FCFSCachedTable *
TofinoModuleGenerator::get_fcfs_cached_table(const EP *ep, const Node *node,
                                             addr_t obj) const {
  const Context &ctx = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

  assert(ds.size() == 1);
  assert((*ds.begin())->type == DSType::FCFS_CACHED_TABLE);

  FCFSCachedTable *cached_table = static_cast<FCFSCachedTable *>(*ds.begin());
  std::unordered_set<DS_ID> deps = tofino_ctx->get_stateful_deps(ep, node);

  if (!tofino_ctx->check_placement(ep, cached_table, deps)) {
    cached_table = nullptr;
  }

  return cached_table;
}

bool TofinoModuleGenerator::can_get_or_build_fcfs_cached_table(
    const EP *ep, const Node *node, addr_t obj, klee::ref<klee::Expr> key,
    int num_entries, int cache_capacity) const {
  FCFSCachedTable *cached_table = nullptr;

  const Context &ctx = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable);

  if (already_placed) {
    cached_table = get_fcfs_cached_table(ep, node, obj);

    if (!cached_table || cached_table->cache_capacity != cache_capacity) {
      return false;
    }

    return true;
  }

  cached_table =
      build_fcfs_cached_table(ep, node, key, num_entries, cache_capacity);

  if (!cached_table) {
    return false;
  }

  delete cached_table;
  return true;
}

symbols_t TofinoModuleGenerator::get_dataplane_state(const EP *ep,
                                                     const Node *node) const {
  const nodes_t &roots = ep->get_target_roots(TargetType::Tofino);
  return get_prev_symbols(node, roots);
}

bool TofinoModuleGenerator::can_place_fcfs_cached_table(
    const EP *ep, const map_coalescing_objs_t &map_objs) const {
  const Context &ctx = ep->get_ctx();
  return ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_FCFSCachedTable) &&
         ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable) &&
         ctx.can_impl_ds(map_objs.vector_key, DSImpl::Tofino_FCFSCachedTable);
}

void TofinoModuleGenerator::place_fcfs_cached_table(
    EP *ep, const Node *node, const map_coalescing_objs_t &map_objs,
    FCFSCachedTable *ds) const {
  assert(ds->type == DSType::FCFS_CACHED_TABLE);
  FCFSCachedTable *cached_table = static_cast<FCFSCachedTable *>(ds);

  Context &ctx = ep->get_mutable_ctx();
  ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);
  ctx.save_ds_impl(map_objs.vector_key, DSImpl::Tofino_FCFSCachedTable);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(ep);
  std::unordered_set<DS_ID> deps = tofino_ctx->get_stateful_deps(ep, node);
  tofino_ctx->place(ep, map_objs.map, cached_table, deps);
}

std::vector<int>
TofinoModuleGenerator::enum_fcfs_cache_cap(int num_entries) const {
  std::vector<int> capacities;

  int cache_capacity = 8;
  while (1) {
    capacities.push_back(cache_capacity);
    cache_capacity *= 2;

    if (cache_capacity > num_entries) {
      break;
    }
  }

  return capacities;
}

hit_rate_t TofinoModuleGenerator::get_fcfs_cache_hr(const EP *ep,
                                                    const Node *node,
                                                    klee::ref<klee::Expr> key,
                                                    int cache_capacity) const {
  const Context &ctx = ep->get_ctx();
  const Profiler &profiler = ctx.get_profiler();
  constraints_t constraints = node->get_ordered_branch_constraints();

  std::optional<FlowStats> flow_stats =
      profiler.get_flow_stats(constraints, key);
  assert(flow_stats.has_value());

  u64 cached_packets = std::min(flow_stats->packets,
                                flow_stats->avg_pkts_per_flow * cache_capacity);
  hit_rate_t hit_rate =
      cached_packets / static_cast<hit_rate_t>(flow_stats->packets);

  // std::cerr << "Pkts/flow: " << flow_stats->avg_pkts_per_flow << "\n";
  // std::cerr << "Flows: " << flow_stats->flows << "\n";
  // std::cerr << "Packets: " << flow_stats->packets << "\n";
  // std::cerr << "Cached packets: " << cached_packets << "\n";
  // std::cerr << "Cache hit rate: " << hit_rate << "\n";

  return hit_rate;
}

} // namespace tofino
