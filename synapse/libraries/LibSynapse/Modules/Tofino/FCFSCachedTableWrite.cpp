#include <LibSynapse/Modules/Tofino/FCFSCachedTableWrite.h>
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
struct fcfs_cached_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> original_key;
  std::vector<klee::ref<klee::Expr>> keys;
  u32 capacity;
  map_coalescing_objs_t map_objs;
};

std::optional<fcfs_cached_table_data_t> build_fcfs_cached_table_data(const BDD *bdd, const Context &ctx, const Call *map_put) {
  const call_t &put_call = map_put->get_call();

  fcfs_cached_table_data_t data;
  data.obj          = expr_addr_to_obj_addr(put_call.args.at("map").expr);
  data.original_key = put_call.args.at("key").in;
  data.capacity     = ctx.get_map_config(data.obj).capacity;

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  data.map_objs = map_objs.value();

  std::vector<BDD::vector_values_t> values;
  if (!data.map_objs.vectors.empty()) {
    values = bdd->get_vector_values_from_map_op(map_put);
  }

  if (values.size() != 0) {
    return {};
  }

  return data;
}

hit_rate_t get_cache_success_estimation_rel(const EP *ep, const BDDNode *node, const BDDNode *map_put, const TargetType target,
                                            klee::ref<klee::Expr> key, u32 cache_capacity) {

  const hit_rate_t fraction                 = ep->get_ctx().get_profiler().get_hr(node);
  const hit_rate_t expected_cached_fraction = TofinoModuleFactory::get_fcfs_cache_success_rate(ep->get_ctx(), map_put, key, cache_capacity);
  return hit_rate_t{fraction * expected_cached_fraction};
}

std::vector<const BDDNode *> get_nodes_to_speculatively_ignore(const EP *ep, const Call *dchain_allocate_new_index,
                                                               const map_coalescing_objs_t &map_objs, klee::ref<klee::Expr> key) {
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

klee::ref<klee::Expr> build_cache_write_success_condition(const symbol_t &cache_write_success) {
  klee::ref<klee::Expr> zero = solver_toolbox.exprBuilder->Constant(0, cache_write_success.expr->getWidth());
  return solver_toolbox.exprBuilder->Ne(cache_write_success.expr, zero);
}

void add_dchain_allocate_new_index_clone_on_cache_write_failed(const EP *ep, BDD *bdd, const BDDNode *dchain_allocate_new_index,
                                                               const Branch *cache_write_branch, BDDNode *&new_on_cache_write_failed) {
  bdd_node_id_t &id                                        = bdd->get_mutable_id();
  BDDNodeManager &manager                                  = bdd->get_mutable_manager();
  BDDNode *dchain_allocate_new_index_on_cache_write_failed = dchain_allocate_new_index->clone(manager, false);
  dchain_allocate_new_index_on_cache_write_failed->recursive_update_ids(id);

  new_on_cache_write_failed =
      bdd->add_cloned_non_branches(cache_write_branch->get_on_false()->get_id(), {dchain_allocate_new_index_on_cache_write_failed});
}

void replicate_hdr_parsing_ops_on_cache_write_failed(const EP *ep, BDD *bdd, const Branch *cache_write_branch, BDDNode *&new_on_cache_write_failed) {
  const BDDNode *on_cache_write_failed = cache_write_branch->get_on_false();

  std::vector<const Call *> prev_borrows =
      on_cache_write_failed->get_prev_functions({"packet_borrow_next_chunk"}, ep->get_target_roots(ep->get_active_target()));

  if (prev_borrows.empty()) {
    return;
  }

  std::vector<const BDDNode *> non_branch_nodes_to_add;
  for (const Call *prev_borrow : prev_borrows) {
    non_branch_nodes_to_add.push_back(prev_borrow);
  }

  new_on_cache_write_failed = bdd->add_cloned_non_branches(on_cache_write_failed->get_id(), non_branch_nodes_to_add);
}

void delete_coalescing_nodes_on_success(const EP *ep, BDD *bdd, BDDNode *on_success, const map_coalescing_objs_t &map_objs, klee::ref<klee::Expr> key,
                                        std::optional<std::vector<klee::ref<klee::Expr>>> &deleted_branch_constraints) {
  const std::vector<const Call *> targets = on_success->get_coalescing_nodes_from_key(key, map_objs);

  for (const BDDNode *target : targets) {
    const Call *call_target = dynamic_cast<const Call *>(target);
    const call_t &call      = call_target->get_call();

    if (call.function_name == "dchain_allocate_new_index") {
      const branch_direction_t index_alloc_check = bdd->find_branch_checking_index_alloc(call_target);
      if (index_alloc_check.branch) {
        assert(!deleted_branch_constraints.has_value() && "Multiple branch checking index allocation detected");
        deleted_branch_constraints = index_alloc_check.branch->get_ordered_branch_constraints();

        klee::ref<klee::Expr> extra_constraint = index_alloc_check.branch->get_condition();

        // If we want to keep the direction on true, we must remove the on false.
        if (index_alloc_check.direction) {
          extra_constraint = solver_toolbox.exprBuilder->Not(extra_constraint);
        }

        deleted_branch_constraints->push_back(extra_constraint);

        const BDD::BranchDeletionAction branch_deletion_action =
            index_alloc_check.direction ? BDD::BranchDeletionAction::KeepOnTrue : BDD::BranchDeletionAction::KeepOnFalse;
        bdd->delete_branch(index_alloc_check.branch->get_id(), branch_deletion_action);
      }
    }

    bdd->delete_non_branch(target->get_id());
  }
}

std::unique_ptr<BDD> branch_bdd_on_cache_write_success(const EP *ep, const BDDNode *dchain_allocate_new_index,
                                                       const fcfs_cached_table_data_t &fcfs_cached_table_data, const symbol_t &cache_write_success,
                                                       klee::ref<klee::Expr> cache_write_success_condition, BDDNode *&on_cache_write_success,
                                                       BDDNode *&on_cache_write_failed,
                                                       std::optional<std::vector<klee::ref<klee::Expr>>> &deleted_branch_constraints) {
  const BDD *old_bdd           = ep->get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const BDDNode *next = dchain_allocate_new_index->get_next();
  assert(next && "No next node");

  dynamic_cast<Call *>(new_bdd->get_mutable_node_by_id(dchain_allocate_new_index->get_id()))->add_local_symbol(cache_write_success);

  Branch *cache_write_branch = new_bdd->add_cloned_branch(next->get_id(), cache_write_success_condition);
  on_cache_write_success     = cache_write_branch->get_mutable_on_true();

  add_dchain_allocate_new_index_clone_on_cache_write_failed(ep, new_bdd.get(), dchain_allocate_new_index, cache_write_branch, on_cache_write_failed);
  replicate_hdr_parsing_ops_on_cache_write_failed(ep, new_bdd.get(), cache_write_branch, on_cache_write_failed);
  delete_coalescing_nodes_on_success(ep, new_bdd.get(), on_cache_write_success, fcfs_cached_table_data.map_objs, fcfs_cached_table_data.original_key,
                                     deleted_branch_constraints);

  return new_bdd;
}

std::unique_ptr<EP> concretize_cached_table_write(const EP *ep, const BDDNode *node, const TargetType target,
                                                  const fcfs_cached_table_data_t &fcfs_cached_table_data, const symbol_t &cache_write_success,
                                                  u32 cache_capacity, const Call *map_put) {
  FCFSCachedTable *cached_table = TofinoModuleFactory::build_or_reuse_fcfs_cached_table(
      ep, node, target, fcfs_cached_table_data.obj, fcfs_cached_table_data.original_key, fcfs_cached_table_data.capacity, cache_capacity);

  if (!cached_table) {
    return nullptr;
  }

  klee::ref<klee::Expr> cache_write_success_condition = build_cache_write_success_condition(cache_write_success);

  Module *module = new FCFSCachedTableWrite(target.instance_id, node, cached_table->id, fcfs_cached_table_data.obj, fcfs_cached_table_data.keys,
                                            cache_write_success);
  EPNode *cached_table_write_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  BDDNode *on_cache_write_success;
  BDDNode *on_cache_write_failed;
  std::optional<std::vector<klee::ref<klee::Expr>>> deleted_branch_constraints;

  std::unique_ptr<BDD> new_bdd =
      branch_bdd_on_cache_write_success(new_ep.get(), node, fcfs_cached_table_data, cache_write_success, cache_write_success_condition,
                                        on_cache_write_success, on_cache_write_failed, deleted_branch_constraints);

  Symbols symbols = TofinoModuleFactory::get_relevant_dataplane_state(ep, node, ep->get_active_target());

  Module *if_module                 = new If(target.instance_id, node, cache_write_success_condition, {cache_write_success_condition});
  Module *then_module               = new Then(target.instance_id, node);
  Module *else_module               = new Else(target.instance_id, node);
  Module *send_to_controller_module = new SendToController(target.instance_id, on_cache_write_failed, symbols);

  EPNode *if_node                 = new EPNode(if_module);
  EPNode *then_node               = new EPNode(then_module);
  EPNode *else_node               = new EPNode(else_module);
  EPNode *send_to_controller_node = new EPNode(send_to_controller_module);

  cached_table_write_node->set_children(if_node);

  if_node->set_prev(cached_table_write_node);
  if_node->set_children(cache_write_success_condition, then_node, else_node);

  then_node->set_prev(if_node);

  else_node->set_prev(if_node);
  else_node->set_children(send_to_controller_node);

  send_to_controller_node->set_prev(else_node);

  const hit_rate_t cache_write_success_estimation_rel =
      get_cache_success_estimation_rel(ep, node, map_put, target, fcfs_cached_table_data.original_key, cache_capacity);

  new_ep->get_mutable_ctx().get_mutable_profiler().insert_relative(new_ep->get_active_leaf().node->get_constraints(), cache_write_success_condition,
                                                                   cache_write_success_estimation_rel);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(fcfs_cached_table_data.map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  ctx.save_ds_impl(fcfs_cached_table_data.map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  if (deleted_branch_constraints.has_value()) {
    ctx.get_mutable_profiler().remove(deleted_branch_constraints.value());
  }

  TofinoContext *tofino_ctx = TofinoModuleFactory::get_mutable_tofino_ctx(new_ep.get(), target);
  tofino_ctx->place(new_ep.get(), node, fcfs_cached_table_data.map_objs.map, cached_table);

  EPLeaf on_cache_write_success_leaf(then_node, on_cache_write_success);
  EPLeaf on_cache_write_failed_leaf(send_to_controller_node, on_cache_write_failed);

  new_ep->process_leaf(cached_table_write_node, {on_cache_write_success_leaf, on_cache_write_failed_leaf});
  new_ep->replace_bdd(std::move(new_bdd));

  const hit_rate_t hr = new_ep->get_ctx().get_profiler().get_hr(send_to_controller_node);
  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_traffic(new_ep->get_node_egress(hr, send_to_controller_node));

  return new_ep;
}
} // namespace

std::optional<spec_impl_t> FCFSCachedTableWriteFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!ep->get_bdd()->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts)) {
    // The cached table read should deal with these cases.
    return {};
  }

  assert(!future_map_puts.empty() && "No future map puts");

  if (future_map_puts.size() > 1) {
    // We do not support multiple map puts yet.
    return {};
  }

  const Call *map_put = future_map_puts[0];

  const std::optional<fcfs_cached_table_data_t> fcfs_cached_table_data = build_fcfs_cached_table_data(ep->get_bdd(), ep->get_ctx(), map_put);
  if (!fcfs_cached_table_data.has_value()) {
    return {};
  }

  if (!ctx.can_impl_ds(fcfs_cached_table_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ctx.can_impl_ds(fcfs_cached_table_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  if (!ep->get_bdd()->is_dchain_used_exclusively_for_linking_maps_with_vectors(fcfs_cached_table_data->map_objs.dchain)) {
    return {};
  }

  if (const EPNode *ep_node_leaf = ep->get_leaf_ep_node_from_bdd_node(node)) {
    if (was_ds_already_used(ep_node_leaf, build_fcfs_cached_table_id(fcfs_cached_table_data->map_objs.map))) {
      return {};
    }
  }

  const std::vector<u32> allowed_cache_capacities = enum_fcfs_cache_cap(fcfs_cached_table_data->capacity);

  hit_rate_t chosen_success_estimation = 0_hr;
  u32 chosen_cache_capacity            = 0;
  bool successfully_placed             = false;

  // We can use a different method for picking the right estimation depending
  // on the time it takes to find a solution.
  for (u32 cache_capacity : allowed_cache_capacities) {
    const hit_rate_t success_estimation =
        get_cache_success_estimation_rel(ep, node, map_put, get_target(), fcfs_cached_table_data->original_key, cache_capacity);

    if (!can_build_or_reuse_fcfs_cached_table(ep, node, get_target(), fcfs_cached_table_data->obj, fcfs_cached_table_data->original_key,
                                              fcfs_cached_table_data->capacity, cache_capacity)) {
      break;
    }

    if (success_estimation > chosen_success_estimation) {
      chosen_success_estimation = success_estimation;
      chosen_cache_capacity     = cache_capacity;
    }

    successfully_placed = true;
  }

  if (!successfully_placed) {
    return {};
  }

  Context new_ctx          = ctx;
  const Profiler &profiler = new_ctx.get_profiler();

  const hit_rate_t fraction         = profiler.get_hr(node);
  const hit_rate_t on_fail_fraction = hit_rate_t{fraction * (1 - chosen_success_estimation)};

  new_ctx.get_mutable_profiler().scale(node->get_ordered_branch_constraints(), chosen_success_estimation.value);
  new_ctx.save_ds_impl(fcfs_cached_table_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  new_ctx.save_ds_impl(fcfs_cached_table_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  new_ctx.get_mutable_perf_oracle().add_controller_traffic(on_fail_fraction);

  spec_impl_t spec_impl(decide(ep, node, {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, chosen_cache_capacity}}), new_ctx);

  const std::vector<const BDDNode *> ignore_nodes =
      get_nodes_to_speculatively_ignore(ep, dchain_allocate_new_index, fcfs_cached_table_data->map_objs, fcfs_cached_table_data->original_key);
  for (const BDDNode *op : ignore_nodes) {
    spec_impl.skip.insert(op->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> FCFSCachedTableWriteFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!ep->get_bdd()->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts)) {
    // The cached table read should deal with these cases.
    return {};
  }

  assert(!future_map_puts.empty() && "No future map puts");

  if (future_map_puts.size() > 1) {
    // We do not support multiple map puts yet.
    return {};
  }

  const Call *map_put = future_map_puts[0];

  const std::optional<fcfs_cached_table_data_t> fcfs_cached_table_data = build_fcfs_cached_table_data(ep->get_bdd(), ep->get_ctx(), map_put);
  if (!fcfs_cached_table_data.has_value()) {
    return {};
  }

  if (!ep->get_bdd()->is_dchain_used_exclusively_for_linking_maps_with_vectors(fcfs_cached_table_data->map_objs.dchain)) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(fcfs_cached_table_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ep->get_ctx().can_impl_ds(fcfs_cached_table_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  if (const EPNode *ep_node_leaf = ep->get_leaf_ep_node_from_bdd_node(node)) {
    if (was_ds_already_used(ep_node_leaf, build_fcfs_cached_table_id(fcfs_cached_table_data->map_objs.map))) {
      return {};
    }
  }

  const symbol_t cache_write_success              = symbol_manager->create_symbol("cache_write_success", 32);
  const std::vector<u32> allowed_cache_capacities = enum_fcfs_cache_cap(fcfs_cached_table_data->capacity);

  std::vector<impl_t> impls;
  for (u32 cache_capacity : allowed_cache_capacities) {
    std::unique_ptr<EP> new_ep =
        concretize_cached_table_write(ep, node, get_target(), fcfs_cached_table_data.value(), cache_write_success, cache_capacity, map_put);
    if (new_ep) {
      impl_t impl = implement(ep, node, std::move(new_ep), {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, cache_capacity}});
      impls.push_back(std::move(impl));
    }
  }

  return impls;
}

std::unique_ptr<Module> FCFSCachedTableWriteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!bdd->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts)) {
    // The cached table read should deal with these cases.
    return {};
  }

  assert(!future_map_puts.empty() && "No future map puts");

  if (future_map_puts.size() > 1) {
    // We do not support multiple map puts yet.
    return {};
  }

  const Call *map_put = future_map_puts[0];

  const std::optional<fcfs_cached_table_data_t> fcfs_cached_table_data = build_fcfs_cached_table_data(bdd, ctx, map_put);
  if (!fcfs_cached_table_data.has_value()) {
    return {};
  }

  if (!ctx.check_ds_impl(fcfs_cached_table_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ctx.check_ds_impl(fcfs_cached_table_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  symbol_t mock_cache_write_failed;

  const std::unordered_set<Tofino::DS *> ds =
      ctx.get_target_ctx<TofinoContext>(get_target())->get_data_structures().get_ds(fcfs_cached_table_data->map_objs.map);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const FCFSCachedTable *fcfs_cached_table = dynamic_cast<const FCFSCachedTable *>(*ds.begin());

  return std::make_unique<FCFSCachedTableWrite>(get_type().instance_id, node, fcfs_cached_table->id, fcfs_cached_table_data->obj,
                                                fcfs_cached_table_data->keys, mock_cache_write_failed);
}

} // namespace Tofino
} // namespace LibSynapse
