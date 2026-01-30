#include <LibSynapse/Modules/Tofino/FCFSCachedTableInsert.h>
#include <LibSynapse/Modules/Tofino/If.h>
#include <LibSynapse/Modules/Tofino/Then.h>
#include <LibSynapse/Modules/Tofino/Else.h>
#include <LibSynapse/Modules/Tofino/SendToController.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::BDDNodeManager;
using LibBDD::Branch;
using LibBDD::branch_direction_t;
using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct fcfs_ct_data_t {
  addr_t obj;
  klee::ref<klee::Expr> original_key;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  u32 capacity;
};

fcfs_ct_data_t get_fcfs_ct_data(const Context &ctx, const Call *map_put) {
  const call_t &put_call = map_put->get_call();

  fcfs_ct_data_t data;
  data.obj          = expr_addr_to_obj_addr(put_call.args.at("map").expr);
  data.original_key = put_call.args.at("key").in;
  data.keys         = TofinoModuleFactory::partition_expr_for_registers(ctx, data.original_key);
  data.value        = put_call.args.at("value").expr;
  data.capacity     = ctx.get_map_config(data.obj).capacity;

  return data;
}

struct pattern_t {
  const Call *map_put;
  const Call *dchain_allocate_new_index;
  branch_direction_t index_alloc_success_direction;
  symbol_t new_index;
  symbol_t index_allocation_success;
};

bool is_write_pattern(const Context &ctx, const BDD *bdd, const BDDNode *node, pattern_t &pattern) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(node);

  pattern.index_alloc_success_direction = bdd->find_branch_checking_index_alloc(dchain_allocate_new_index);
  if (pattern.index_alloc_success_direction.branch == nullptr) {
    return false;
  }

  const BDDNode *on_index_allocation_success = pattern.index_alloc_success_direction.get_success_node();
  if (on_index_allocation_success->get_type() != BDDNodeType::Call) {
    return false;
  }

  const addr_t obj = expr_addr_to_obj_addr(dchain_allocate_new_index->get_call().args.at("chain").expr);

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(obj);
  if (!map_objs.has_value()) {
    return false;
  }

  std::vector<const Call *> future_map_puts;
  if (!bdd->is_map_update_with_dchain(dchain_allocate_new_index, map_objs.value(), future_map_puts) || future_map_puts.size() != 1) {
    return false;
  }

  pattern.map_put                   = future_map_puts[0];
  pattern.dchain_allocate_new_index = dchain_allocate_new_index;
  pattern.new_index                 = dchain_allocate_new_index->get_local_symbol("new_index");
  pattern.index_allocation_success  = dchain_allocate_new_index->get_local_symbol("not_out_of_space");

  return true;
}

std::vector<const BDDNode *> get_nodes_to_speculatively_ignore(const EP *ep, const Call *dchain_allocate_new_index, map_coalescing_objs_t map_objs,
                                                               klee::ref<klee::Expr> key) {
  // Don't ignore the vectors.
  map_objs.vectors.clear();

  std::vector<const Call *> coalescing_nodes = dchain_allocate_new_index->get_coalescing_nodes_from_key(key, map_objs);

  std::vector<const BDDNode *> nodes_to_ignore;
  for (const Call *coalescing_node : coalescing_nodes) {
    nodes_to_ignore.push_back(coalescing_node);
  }

  const branch_direction_t index_alloc_check = ep->get_bdd()->find_branch_checking_index_alloc(dchain_allocate_new_index);
  if (index_alloc_check.branch) {
    nodes_to_ignore.push_back(index_alloc_check.branch);

    const BDDNode *next = index_alloc_check.direction ? index_alloc_check.branch->get_on_false() : index_alloc_check.branch->get_on_true();

    next->visit_nodes([&nodes_to_ignore](const BDDNode *node) {
      nodes_to_ignore.push_back(node);
      return BDDNodeVisitAction::Continue;
    });
  }

  return nodes_to_ignore;
}

klee::ref<klee::Expr> build_cached_insert_success_condition(const symbol_t &cached_insert_success) {
  klee::ref<klee::Expr> zero = solver_toolbox.exprBuilder->Constant(0, cached_insert_success.expr->getWidth());
  return solver_toolbox.exprBuilder->Ne(cached_insert_success.expr, zero);
}

struct bddnode_translations_pair_t {
  BDDNode *new_node;
  std::vector<symbol_translation_t> translations;
};

bddnode_translations_pair_t add_dchain_allocate_new_index_clone_on_cached_insert_failed(BDD *bdd, const BDDNode *dchain_allocate_new_index,
                                                                                        Branch *cached_insert_branch) {
  bddnode_translations_pair_t bddnode_translations_pair;

  bdd_node_id_t &id       = bdd->get_mutable_id();
  BDDNodeManager &manager = bdd->get_mutable_manager();

  Call *dchain_allocate_new_index_on_cached_insert_failed = dynamic_cast<Call *>(dchain_allocate_new_index->clone(manager, true));
  dchain_allocate_new_index_on_cached_insert_failed->recursive_update_ids(id);

  bddnode_translations_pair.new_node     = dchain_allocate_new_index_on_cached_insert_failed;
  bddnode_translations_pair.translations = dchain_allocate_new_index_on_cached_insert_failed->sync_local_symbols_and_recursively_update_children();

  cached_insert_branch->set_on_false(dchain_allocate_new_index_on_cached_insert_failed);
  dchain_allocate_new_index_on_cached_insert_failed->set_prev(cached_insert_branch);

  return bddnode_translations_pair;
}

BDDNode *send_to_controller_on_cached_insert_failed(BDD *bdd, const Branch *cached_insert_branch) {
  const BDDNode *on_cached_insert_failed = cached_insert_branch->get_on_false();
  symbol_t symbol_reordering_barrier     = bdd->get_reordering_barrier_symbol();
  return bdd->add_new_symbol_generator_function(
      on_cached_insert_failed->get_id(), SendToController::force_send_to_controller_bdd_node_function_name(), Symbols({symbol_reordering_barrier}));
}

BDDNode *delete_coalescing_nodes_and_alloc_failure_on_success(BDD *bdd, BDDNode *on_success, const pattern_t &pattern, map_coalescing_objs_t map_objs,
                                                              klee::ref<klee::Expr> key,
                                                              std::vector<klee::ref<klee::Expr>> &deleted_branch_constraints) {
  // Don't remove the vectors, we only care about the map and dchain for FCFS CT.
  map_objs.vectors.clear();

  const std::vector<const Call *> targets = on_success->get_coalescing_nodes_from_key(key, map_objs);
  for (const BDDNode *target : targets) {
    BDDNode *new_node = bdd->delete_non_branch(target->get_id());
    if (target->get_id() == on_success->get_id()) {
      on_success = new_node;
    }
  }

  const branch_direction_t index_alloc_check = bdd->find_branch_checking_index_alloc(pattern.dchain_allocate_new_index, on_success);
  if (index_alloc_check.branch) {
    deleted_branch_constraints = index_alloc_check.branch->get_ordered_branch_constraints();

    klee::ref<klee::Expr> extra_constraint = index_alloc_check.branch->get_condition();

    // If we want to keep the direction on true, we must remove the on false.
    if (index_alloc_check.direction) {
      extra_constraint = solver_toolbox.exprBuilder->Not(extra_constraint);
    }

    deleted_branch_constraints.push_back(extra_constraint);

    const BDD::BranchDeletionAction branch_deletion_action =
        index_alloc_check.direction ? BDD::BranchDeletionAction::KeepOnTrue : BDD::BranchDeletionAction::KeepOnFalse;
    BDDNode *new_node = bdd->delete_branch(index_alloc_check.branch->get_id(), branch_deletion_action);
    if (index_alloc_check.branch->get_id() == on_success->get_id()) {
      on_success = new_node;
    }
  }

  return on_success;
}

struct rebuilt_bdd_result_t {
  std::unique_ptr<BDD> bdd;
  BDDNode *on_cached_insert_success;
  BDDNode *on_cached_insert_failed;
};

rebuilt_bdd_result_t rebuild_bdd(EP *new_ep, const pattern_t &pattern, const fcfs_ct_data_t &fcfs_ct_data,
                                 const map_coalescing_objs_t &map_coalescing_objs, const symbol_t &cached_insert_success,
                                 klee::ref<klee::Expr> cached_insert_success_condition, u32 cache_capacity) {
  rebuilt_bdd_result_t result;

  const BDD *old_bdd = new_ep->get_bdd();
  result.bdd         = std::make_unique<BDD>(*old_bdd);

  dynamic_cast<Call *>(result.bdd->get_mutable_node_by_id(pattern.dchain_allocate_new_index->get_id()))->add_local_symbol(cached_insert_success);

  Branch *cached_insert_branch =
      result.bdd->add_cloned_branch(pattern.dchain_allocate_new_index->get_next()->get_id(), cached_insert_success_condition);
  result.on_cached_insert_success = cached_insert_branch->get_mutable_on_true();

  const bddnode_translations_pair_t bddnode_translations_pair =
      add_dchain_allocate_new_index_clone_on_cached_insert_failed(result.bdd.get(), pattern.dchain_allocate_new_index, cached_insert_branch);

  result.on_cached_insert_failed = bddnode_translations_pair.new_node;
  result.on_cached_insert_failed = send_to_controller_on_cached_insert_failed(result.bdd.get(), cached_insert_branch);

  std::vector<klee::ref<klee::Expr>> deleted_branch_constraints;
  result.on_cached_insert_success = delete_coalescing_nodes_and_alloc_failure_on_success(
      result.bdd.get(), result.on_cached_insert_success, pattern, map_coalescing_objs, fcfs_ct_data.original_key, deleted_branch_constraints);

  const Call *map_get          = pattern.map_put->get_past_map_get_from_map_put();
  const Call *target_for_stats = map_get ? map_get : pattern.map_put;

  const hit_rate_t cache_hit_rate =
      TofinoModuleFactory::get_fcfs_ct_cache_hit_rate(new_ep->get_ctx(), target_for_stats, fcfs_ct_data.original_key, cache_capacity);

  new_ep->get_mutable_ctx().get_mutable_profiler().insert_relative(pattern.dchain_allocate_new_index->get_ordered_branch_constraints(),
                                                                   cached_insert_success_condition, cache_hit_rate);
  new_ep->get_mutable_ctx().get_mutable_profiler().translate(result.bdd->get_mutable_symbol_manager(), bddnode_translations_pair.new_node,
                                                             bddnode_translations_pair.translations);

  if (!deleted_branch_constraints.empty()) {
    new_ep->get_mutable_ctx().get_mutable_profiler().remove(deleted_branch_constraints);
  }

  return result;
}

std::unique_ptr<EP> concretize(const EP *ep, const BDDNode *node, const pattern_t &pattern, const fcfs_ct_data_t &fcfs_ct_data,
                               const map_coalescing_objs_t &map_coalescing_objs, const symbol_t &cached_insert_success, u32 cache_capacity,
                               const Call *map_put) {
  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  FCFSCachedTable *fcfs_cached_set = TofinoModuleFactory::build_or_reuse_fcfs_ct(
      new_ep.get(), node, map_coalescing_objs.map, fcfs_ct_data.original_key, fcfs_ct_data.capacity, cache_capacity, false);
  if (!fcfs_cached_set) {
    return nullptr;
  }

  klee::ref<klee::Expr> cached_insert_success_condition = build_cached_insert_success_condition(cached_insert_success);
  rebuilt_bdd_result_t rebuilt_bdd_result =
      rebuild_bdd(new_ep.get(), pattern, fcfs_ct_data, map_coalescing_objs, cached_insert_success, cached_insert_success_condition, cache_capacity);

  Module *module =
      new FCFSCachedTableInsert(node, fcfs_cached_set->id, fcfs_ct_data.obj, fcfs_ct_data.keys, fcfs_ct_data.value, cached_insert_success);
  Module *if_module   = new If(node, cached_insert_success_condition, {cached_insert_success_condition});
  Module *then_module = new Then(node);
  Module *else_module = new Else(node);

  EPNode *cached_set_insert_node = new EPNode(module);
  EPNode *if_node                = new EPNode(if_module);
  EPNode *then_node              = new EPNode(then_module);
  EPNode *else_node              = new EPNode(else_module);

  cached_set_insert_node->set_children(if_node);

  if_node->set_prev(cached_set_insert_node);
  if_node->set_children(cached_insert_success_condition, then_node, else_node);

  then_node->set_prev(if_node);
  else_node->set_prev(if_node);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(node->get_id(), map_coalescing_objs.map, DSImpl::Tofino_FCFSCachedTable);
  ctx.save_ds_impl(node->get_id(), map_coalescing_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  TofinoContext *tofino_ctx = TofinoModuleFactory::get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, map_coalescing_objs.map, fcfs_cached_set);

  EPLeaf on_cached_insert_success_leaf(then_node, rebuilt_bdd_result.on_cached_insert_success);
  EPLeaf on_cached_insert_failed_leaf(else_node, rebuilt_bdd_result.on_cached_insert_failed);

  new_ep->process_leaf(cached_set_insert_node, {on_cached_insert_success_leaf, on_cached_insert_failed_leaf});
  new_ep->replace_bdd(std::move(rebuilt_bdd_result.bdd));

  return new_ep;
}
} // namespace

std::optional<spec_impl_t> FCFSCachedTableInsertFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  pattern_t pattern;
  if (!is_write_pattern(speculations.ctx, ep->get_bdd(), node, pattern)) {
    return {};
  }

  const fcfs_ct_data_t data = get_fcfs_ct_data(ep->get_ctx(), pattern.map_put);

  const std::optional<map_coalescing_objs_t> map_coalescing_objs = ep->get_ctx().get_map_coalescing_objs(data.obj);
  if (!map_coalescing_objs.has_value()) {
    return {};
  }

  if (!speculations.ctx.can_impl_ds(map_coalescing_objs->map, DSImpl::Tofino_FCFSCachedTable) ||
      !speculations.ctx.can_impl_ds(map_coalescing_objs->dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  if (speculations.ctx.get_ds_usage_counts().contains({map_coalescing_objs->map, DSImpl::Tofino_FCFSCachedTable}) &&
      speculations.ctx.get_ds_usage_counts().at({map_coalescing_objs->map, DSImpl::Tofino_FCFSCachedTable}) >= 2) {
    return {};
  }

  bool requires_recirculation = false;
  if (was_ds_already_used(ep->get_leaf_ep_node_from_bdd_node(node), speculations, node, map_coalescing_objs->map, DSImpl::Tofino_FCFSCachedTable,
                          build_fcfs_ct_id(map_coalescing_objs->map))) {
    requires_recirculation = true;
  }

  hit_rate_t chosen_success_estimation = 0_hr;
  u32 chosen_cache_capacity            = 0;
  bool successfully_placed             = false;

  const Call *map_get          = pattern.map_put->get_past_map_get_from_map_put();
  const Call *target_for_stats = map_get ? map_get : pattern.map_put;

  std::vector<u32> allowed_cache_capacities = enum_fcfs_ct_cache_capacities(data.capacity);
  std::sort(allowed_cache_capacities.begin(), allowed_cache_capacities.end(), std::greater<int>());

  for (u32 cache_capacity : allowed_cache_capacities) {
    const hit_rate_t cache_hit_rate =
        TofinoModuleFactory::get_fcfs_ct_cache_hit_rate(ep->get_ctx(), target_for_stats, data.original_key, cache_capacity);

    if (!can_build_or_reuse_fcfs_ct(ep, node, map_coalescing_objs->map, data.original_key, data.capacity, cache_capacity, false)) {
      continue;
    }

    if (!successfully_placed || cache_hit_rate > chosen_success_estimation) {
      chosen_success_estimation = cache_hit_rate;
      chosen_cache_capacity     = cache_capacity;
    }

    successfully_placed = true;
  }

  if (!successfully_placed) {
    return {};
  }

  Context new_ctx = speculations.ctx;

  new_ctx.save_ds_impl(node->get_id(), map_coalescing_objs->map, DSImpl::Tofino_FCFSCachedTable);
  new_ctx.save_ds_impl(node->get_id(), map_coalescing_objs->dchain, DSImpl::Tofino_FCFSCachedTable);

  if (requires_recirculation) {
    new_ctx.get_mutable_perf_oracle().add_recirculated_traffic(
        ep->get_speculative_node_egress(new_ctx.get_profiler().get_hr(node), node, speculations));
  }

  speculate_sending_to_controller(ep, node, new_ctx, speculations, 1_hr - chosen_success_estimation, requires_recirculation);

  spec_impl_t spec_impl(decide(ep, node, {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, chosen_cache_capacity}}), new_ctx);

  const std::vector<const BDDNode *> ignore_nodes =
      get_nodes_to_speculatively_ignore(ep, pattern.dchain_allocate_new_index, map_coalescing_objs.value(), data.original_key);
  for (const BDDNode *op : ignore_nodes) {
    spec_impl.skip.insert(op->get_id());
  }
  spec_impl.recirculated = requires_recirculation;

  return spec_impl;
}

std::vector<impl_t> FCFSCachedTableInsertFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  pattern_t pattern;
  if (!is_write_pattern(ep->get_ctx(), ep->get_bdd(), node, pattern)) {
    return {};
  }

  const fcfs_ct_data_t data = get_fcfs_ct_data(ep->get_ctx(), pattern.map_put);

  const std::optional<map_coalescing_objs_t> map_coalescing_objs = ep->get_ctx().get_map_coalescing_objs(data.obj);
  if (!map_coalescing_objs.has_value()) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(map_coalescing_objs->map, DSImpl::Tofino_FCFSCachedTable) ||
      !ep->get_ctx().can_impl_ds(map_coalescing_objs->dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  if (ep->get_ctx().get_ds_usage_counts().contains({map_coalescing_objs->map, DSImpl::Tofino_FCFSCachedTable}) &&
      ep->get_ctx().get_ds_usage_counts().at({map_coalescing_objs->map, DSImpl::Tofino_FCFSCachedTable}) >= 2) {
    return {};
  }

  if (was_ds_already_used(ep->get_leaf_ep_node_from_bdd_node(node), build_fcfs_ct_id(map_coalescing_objs->map))) {
    return {};
  }

  const symbol_t cached_insert_success = symbol_manager->create_symbol("cached_insert_success", 32);

  std::vector<impl_t> impls;
  for (u32 cache_capacity : enum_fcfs_ct_cache_capacities(data.capacity)) {
    if (!can_build_or_reuse_fcfs_ct(ep, node, map_coalescing_objs->map, data.original_key, data.capacity, cache_capacity, false)) {
      continue;
    }

    std::unique_ptr<EP> new_ep =
        concretize(ep, node, pattern, data, map_coalescing_objs.value(), cached_insert_success, cache_capacity, pattern.map_put);
    if (new_ep) {
      impl_t impl = implement(ep, node, std::move(new_ep), {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, cache_capacity}});
      impls.push_back(std::move(impl));
    }
  }

  return impls;
}

std::unique_ptr<Module> FCFSCachedTableInsertFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  pattern_t pattern;
  if (!is_write_pattern(ctx, bdd, node, pattern)) {
    return {};
  }

  const fcfs_ct_data_t data = get_fcfs_ct_data(ctx, pattern.map_put);

  const std::optional<map_coalescing_objs_t> map_coalescing_objs = ctx.get_map_coalescing_objs(data.obj);
  if (!map_coalescing_objs.has_value()) {
    return {};
  }

  if (!ctx.check_ds_impl(map_coalescing_objs->map, DSImpl::Tofino_FCFSCachedTable) ||
      !ctx.check_ds_impl(map_coalescing_objs->dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  if (std::any_of(map_coalescing_objs->vectors.begin(), map_coalescing_objs->vectors.end(),
                  [&](addr_t vector) { return !ctx.can_impl_ds(vector, DSImpl::Tofino_FCFSCachedTable); })) {
    return {};
  }

  symbol_t mock_cached_insert_failed;

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(map_coalescing_objs->map);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const FCFSCachedTable *fcfs_ct = dynamic_cast<const FCFSCachedTable *>(*ds.begin());

  return std::make_unique<FCFSCachedTableInsert>(node, fcfs_ct->id, data.obj, data.keys, data.value, mock_cached_insert_failed);
}

} // namespace Tofino
} // namespace LibSynapse