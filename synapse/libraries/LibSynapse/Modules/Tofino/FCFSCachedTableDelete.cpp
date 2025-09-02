#include <LibSynapse/Modules/Tofino/FCFSCachedTableDelete.h>
#include <LibSynapse/Modules/Tofino/If.h>
#include <LibSynapse/Modules/Tofino/Then.h>
#include <LibSynapse/Modules/Tofino/Else.h>
#include <LibSynapse/Modules/Tofino/SendToController.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Branch;
using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct fcfs_cached_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  u32 capacity;

  fcfs_cached_table_data_t(const Context &ctx, const Call *map_erase) {
    const call_t &call = map_erase->get_call();
    assert(call.function_name == "map_erase" && "Expected map_erase");

    obj      = expr_addr_to_obj_addr(call.args.at("map").expr);
    key      = call.args.at("key").in;
    capacity = ctx.get_map_config(obj).capacity;
  }
};

hit_rate_t get_cache_op_success_probability(const EP *ep, const BDDNode *node, klee::ref<klee::Expr> key, u32 cache_capacity) {
  const hit_rate_t cache_hit_rate = TofinoModuleFactory::get_fcfs_cache_success_rate(ep->get_ctx(), node, key, cache_capacity);
  return cache_hit_rate;
}

std::vector<const BDDNode *> get_future_related_nodes(const EP *ep, const BDDNode *node, const map_coalescing_objs_t &cached_table_data) {
  std::vector<const Call *> ops = node->get_future_functions({"dchain_free_index", "map_erase"});

  std::vector<const BDDNode *> related_ops;
  for (const Call *op : ops) {
    const call_t &call = op->get_call();

    if (call.function_name == "dchain_free_index") {
      klee::ref<klee::Expr> obj_expr = call.args.at("chain").expr;
      addr_t obj                     = expr_addr_to_obj_addr(obj_expr);

      if (obj != cached_table_data.dchain) {
        continue;
      }
    } else if (call.function_name == "map_erase") {
      klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
      addr_t obj                     = expr_addr_to_obj_addr(obj_expr);

      if (obj != cached_table_data.map) {
        continue;
      }
    }

    related_ops.push_back(op);
  }

  return related_ops;
}

void replicate_hdr_parsing_ops_on_cache_delete_failed(const EP *ep, BDD *bdd, const Branch *cache_delete_branch,
                                                      BDDNode *&new_on_cache_delete_failed) {
  const BDDNode *on_cache_delete_failed = cache_delete_branch->get_on_false();

  std::vector<const Call *> prev_borrows =
      on_cache_delete_failed->get_prev_functions({"packet_borrow_next_chunk"}, ep->get_target_roots(ep->get_active_target()));

  if (prev_borrows.empty()) {
    return;
  }

  std::vector<const BDDNode *> non_branch_nodes_to_add;
  for (const Call *prev_borrow : prev_borrows) {
    non_branch_nodes_to_add.push_back(prev_borrow);
  }

  new_on_cache_delete_failed = bdd->add_cloned_non_branches(on_cache_delete_failed->get_id(), non_branch_nodes_to_add);
}

std::unique_ptr<BDD> branch_bdd_on_cache_delete_success(const EP *ep, const BDDNode *map_erase, const fcfs_cached_table_data_t &cached_table_data,
                                                        klee::ref<klee::Expr> cache_delete_success_condition, BDDNode *&on_cache_delete_success,
                                                        BDDNode *&on_cache_delete_failed,
                                                        std::optional<std::vector<klee::ref<klee::Expr>>> &deleted_branch_constraints) {
  const BDD *old_bdd           = ep->get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const BDDNode *next = map_erase->get_next();
  assert(next && "Next node is null");

  Branch *cache_delete_branch = new_bdd->add_cloned_branch(next->get_id(), cache_delete_success_condition);

  on_cache_delete_success = cache_delete_branch->get_mutable_on_true();
  on_cache_delete_failed  = cache_delete_branch->get_mutable_on_false();

  replicate_hdr_parsing_ops_on_cache_delete_failed(ep, new_bdd.get(), cache_delete_branch, on_cache_delete_failed);

  return new_bdd;
}

klee::ref<klee::Expr> build_cache_delete_success_condition(const symbol_t &cache_delete_failed) {
  klee::ref<klee::Expr> zero = solver_toolbox.exprBuilder->Constant(0, cache_delete_failed.expr->getWidth());
  return solver_toolbox.exprBuilder->Eq(cache_delete_failed.expr, zero);
}

std::unique_ptr<EP> concretize_cached_table_delete(const EP *ep, const Call *map_erase, const TargetType target, const ModuleType type,
                                                   const map_coalescing_objs_t &map_objs, const fcfs_cached_table_data_t &cached_table_data,
                                                   const symbol_t &cache_delete_failed, u32 cache_capacity) {
  FCFSCachedTable *cached_table = TofinoModuleFactory::build_or_reuse_fcfs_cached_table(
      ep, map_erase, target, cached_table_data.obj, cached_table_data.key, cached_table_data.capacity, cache_capacity);

  if (!cached_table) {
    return nullptr;
  }

  klee::ref<klee::Expr> cache_delete_success_condition = build_cache_delete_success_condition(cache_delete_failed);

  Module *module = new FCFSCachedTableDelete(ep->get_placement(map_erase->get_id()), map_erase, cached_table->id, cached_table_data.obj,
                                             cached_table_data.key, cache_delete_failed);
  EPNode *cached_table_delete_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  BDDNode *on_cache_delete_success;
  BDDNode *on_cache_delete_failed;
  std::optional<std::vector<klee::ref<klee::Expr>>> deleted_branch_constraints;

  std::unique_ptr<BDD> new_bdd = branch_bdd_on_cache_delete_success(new_ep.get(), map_erase, cached_table_data, cache_delete_success_condition,
                                                                    on_cache_delete_success, on_cache_delete_failed, deleted_branch_constraints);

  Symbols symbols = TofinoModuleFactory::get_relevant_dataplane_state(ep, map_erase, ep->get_active_target());

  Module *if_module   = new If(ep->get_placement(map_erase->get_id()), map_erase, cache_delete_success_condition, {cache_delete_success_condition});
  Module *then_module = new Then(ep->get_placement(map_erase->get_id()), map_erase);
  Module *else_module = new Else(ep->get_placement(map_erase->get_id()), map_erase);
  Module *send_to_controller_module = new SendToController(ep->get_placement(on_cache_delete_failed->get_id()), on_cache_delete_failed, symbols);

  EPNode *if_node                 = new EPNode(if_module);
  EPNode *then_node               = new EPNode(then_module);
  EPNode *else_node               = new EPNode(else_module);
  EPNode *send_to_controller_node = new EPNode(send_to_controller_module);

  cached_table_delete_node->set_children(if_node);

  if_node->set_prev(cached_table_delete_node);
  if_node->set_children(cache_delete_success_condition, then_node, else_node);

  then_node->set_prev(if_node);

  else_node->set_prev(if_node);
  else_node->set_children(send_to_controller_node);

  send_to_controller_node->set_prev(else_node);

  const hit_rate_t cache_success_probability = get_cache_op_success_probability(ep, map_erase, cached_table_data.key, cache_capacity);

  new_ep->get_mutable_ctx().get_mutable_profiler().insert_relative(new_ep->get_active_leaf().node->get_constraints(), cache_delete_success_condition,
                                                                   cache_success_probability);

  if (deleted_branch_constraints.has_value()) {
    new_ep->get_mutable_ctx().get_mutable_profiler().remove(deleted_branch_constraints.value());
  }

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  TofinoContext *tofino_ctx = TofinoModuleFactory::get_mutable_tofino_ctx(new_ep.get(), target);
  tofino_ctx->place(new_ep.get(), map_erase, map_objs.map, cached_table);

  EPLeaf on_cache_delete_success_leaf(then_node, on_cache_delete_success);
  EPLeaf on_cache_delete_failed_leaf(send_to_controller_node, on_cache_delete_failed);

  new_ep->process_leaf(cached_table_delete_node, {on_cache_delete_success_leaf, on_cache_delete_failed_leaf});
  new_ep->replace_bdd(std::move(new_bdd));

  const hit_rate_t hr = new_ep->get_ctx().get_profiler().get_hr(send_to_controller_node);
  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_traffic(new_ep->get_node_egress(hr, send_to_controller_node));

  return new_ep;
}
} // namespace

std::optional<spec_impl_t> FCFSCachedTableDeleteFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_erase = dynamic_cast<const Call *>(node);
  const call_t &call    = map_erase->get_call();

  if (call.function_name != "map_erase") {
    return {};
  }

  const fcfs_cached_table_data_t cached_table_data(ctx, map_erase);

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(cached_table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ctx.can_impl_ds(map_objs->map, DSImpl::Tofino_FCFSCachedTable) || !ctx.can_impl_ds(map_objs->dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const std::vector<u32> allowed_cache_capacities = enum_fcfs_cache_cap(cached_table_data.capacity);

  hit_rate_t chosen_cache_success_probability = 0_hr;
  u32 chosen_cache_capacity                   = 0;
  bool successfully_placed                    = false;

  // We can use a different method for picking the right estimation depending
  // on the time it takes to find a solution.
  for (u32 cache_capacity : allowed_cache_capacities) {
    const hit_rate_t cache_success_probability = get_cache_op_success_probability(ep, node, cached_table_data.key, cache_capacity);

    if (!can_get_or_build_fcfs_cached_table(ep, node, target, cached_table_data.obj, cached_table_data.key, cached_table_data.capacity,
                                            cache_capacity)) {
      break;
    }

    if (cache_success_probability > chosen_cache_success_probability) {
      chosen_cache_success_probability = cache_success_probability;
      chosen_cache_capacity            = cache_capacity;
    }

    successfully_placed = true;
  }

  if (!successfully_placed) {
    return {};
  }

  Context new_ctx          = ctx;
  const Profiler &profiler = new_ctx.get_profiler();

  const hit_rate_t fraction         = profiler.get_hr(node);
  const hit_rate_t on_fail_fraction = hit_rate_t{fraction * (1 - chosen_cache_success_probability)};

  // FIXME: not using profiler cache.
  std::vector<klee::ref<klee::Expr>> constraints = node->get_ordered_branch_constraints();

  new_ctx.get_mutable_profiler().scale(constraints, chosen_cache_success_probability.value);
  new_ctx.save_ds_impl(map_objs->map, DSImpl::Tofino_FCFSCachedTable);
  new_ctx.save_ds_impl(map_objs->dchain, DSImpl::Tofino_FCFSCachedTable);

  new_ctx.get_mutable_perf_oracle().add_controller_traffic(on_fail_fraction);

  spec_impl_t spec_impl(decide(ep, node, {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, chosen_cache_capacity}}), new_ctx);

  std::vector<const BDDNode *> ignore_nodes = get_future_related_nodes(ep, node, map_objs.value());

  for (const BDDNode *op : ignore_nodes) {
    spec_impl.skip.insert(op->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> FCFSCachedTableDeleteFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_erase = dynamic_cast<const Call *>(node);
  const call_t &call    = map_erase->get_call();

  if (call.function_name != "map_erase") {
    return {};
  }

  const fcfs_cached_table_data_t cached_table_data(ep->get_ctx(), map_erase);

  const std::optional<map_coalescing_objs_t> map_objs = ep->get_ctx().get_map_coalescing_objs(cached_table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(map_objs->map, DSImpl::Tofino_FCFSCachedTable) ||
      !ep->get_ctx().can_impl_ds(map_objs->dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const symbol_t cache_delete_failed              = symbol_manager->create_symbol("cache_delete_failed", 32);
  const std::vector<u32> allowed_cache_capacities = enum_fcfs_cache_cap(cached_table_data.capacity);

  std::vector<impl_t> impls;
  for (u32 cache_capacity : allowed_cache_capacities) {
    std::unique_ptr<EP> new_ep = concretize_cached_table_delete(ep, map_erase, get_target(), get_type(), map_objs.value(), cached_table_data,
                                                                cache_delete_failed, cache_capacity);
    if (new_ep) {
      impl_t impl = implement(ep, map_erase, std::move(new_ep), {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, cache_capacity}});
      impls.push_back(std::move(impl));
    }
  }

  return impls;
}

std::unique_ptr<Module> FCFSCachedTableDeleteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_erase = dynamic_cast<const Call *>(node);
  const call_t &call    = map_erase->get_call();

  if (call.function_name != "map_erase") {
    return {};
  }

  const fcfs_cached_table_data_t cached_table_data(ctx, map_erase);

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(cached_table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_FCFSCachedTable) || !ctx.check_ds_impl(map_objs->dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const symbol_t mock_cache_delete_failed;

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>(target)->get_data_structures().get_ds(map_objs->map);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const FCFSCachedTable *fcfs_cached_table = dynamic_cast<const FCFSCachedTable *>(*ds.begin());

  return std::make_unique<FCFSCachedTableDelete>(get_type().instance_id, node, fcfs_cached_table->id, cached_table_data.obj, cached_table_data.key,
                                                 mock_cache_delete_failed);
}

} // namespace Tofino
} // namespace LibSynapse