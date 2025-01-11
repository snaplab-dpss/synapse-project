#include "fcfs_cached_table_delete.h"

#include "if.h"
#include "else.h"
#include "then.h"
#include "send_to_controller.h"

namespace synapse {
namespace tofino {
namespace {
struct fcfs_cached_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  u32 num_entries;

  fcfs_cached_table_data_t(const EP *ep, const Call *map_erase) {
    const call_t &call = map_erase->get_call();
    assert(call.function_name == "map_erase" && "Expected map_erase");

    obj         = expr_addr_to_obj_addr(call.args.at("map").expr);
    key         = call.args.at("key").in;
    num_entries = ep->get_ctx().get_map_config(obj).capacity;
  }
};

hit_rate_t get_cache_op_success_probability(const EP *ep, const Node *node, klee::ref<klee::Expr> key, u32 cache_capacity) {
  hit_rate_t cache_hit_rate = TofinoModuleFactory::get_fcfs_cache_success_rate(ep->get_ctx(), node, key, cache_capacity);
  return cache_hit_rate;
}

std::vector<const Node *> get_future_related_nodes(const EP *ep, const Node *node,
                                                   const map_coalescing_objs_t &cached_table_data) {
  std::vector<const Call *> ops = node->get_future_functions({"dchain_free_index", "map_erase"});

  std::vector<const Node *> related_ops;
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
                                                      Node *&new_on_cache_delete_failed) {
  const Node *on_cache_delete_failed = cache_delete_branch->get_on_false();

  std::vector<const Call *> prev_borrows =
      on_cache_delete_failed->get_prev_functions({"packet_borrow_next_chunk"}, ep->get_target_roots(ep->get_active_target()));

  if (prev_borrows.empty()) {
    return;
  }

  std::vector<const Node *> non_branch_nodes_to_add;
  for (const Call *prev_borrow : prev_borrows) {
    non_branch_nodes_to_add.push_back(prev_borrow);
  }

  new_on_cache_delete_failed = bdd->clone_and_add_non_branches(on_cache_delete_failed, non_branch_nodes_to_add);
}

std::unique_ptr<BDD> branch_bdd_on_cache_delete_success(const EP *ep, const Node *map_erase,
                                                        const fcfs_cached_table_data_t &cached_table_data,
                                                        klee::ref<klee::Expr> cache_delete_success_condition,
                                                        Node *&on_cache_delete_success, Node *&on_cache_delete_failed,
                                                        std::optional<constraints_t> &deleted_branch_constraints) {
  const BDD *old_bdd           = ep->get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const Node *next = map_erase->get_next();
  assert(next && "Next node is null");

  Branch *cache_delete_branch = new_bdd->clone_and_add_branch(next, cache_delete_success_condition);

  on_cache_delete_success = cache_delete_branch->get_mutable_on_true();
  on_cache_delete_failed  = cache_delete_branch->get_mutable_on_false();

  replicate_hdr_parsing_ops_on_cache_delete_failed(ep, new_bdd.get(), cache_delete_branch, on_cache_delete_failed);

  return new_bdd;
}

klee::ref<klee::Expr> build_cache_delete_success_condition(const symbol_t &cache_delete_failed) {
  klee::ref<klee::Expr> zero = solver_toolbox.exprBuilder->Constant(0, cache_delete_failed.expr->getWidth());
  return solver_toolbox.exprBuilder->Eq(cache_delete_failed.expr, zero);
}

EP *concretize_cached_table_delete(const EP *ep, const Call *map_erase, const map_coalescing_objs_t &map_objs,
                                   const fcfs_cached_table_data_t &cached_table_data, const symbol_t &cache_delete_failed,
                                   u32 cache_capacity) {
  FCFSCachedTable *cached_table = TofinoModuleFactory::build_or_reuse_fcfs_cached_table(
      ep, map_erase, cached_table_data.obj, cached_table_data.key, cached_table_data.num_entries, cache_capacity);

  if (!cached_table) {
    return nullptr;
  }

  klee::ref<klee::Expr> cache_delete_success_condition = build_cache_delete_success_condition(cache_delete_failed);

  Module *module =
      new FCFSCachedTableDelete(map_erase, cached_table->id, cached_table_data.obj, cached_table_data.key, cache_delete_failed);
  EPNode *cached_table_delete_node = new EPNode(module);

  EP *new_ep = new EP(*ep);

  Node *on_cache_delete_success;
  Node *on_cache_delete_failed;
  std::optional<constraints_t> deleted_branch_constraints;

  std::unique_ptr<BDD> new_bdd =
      branch_bdd_on_cache_delete_success(new_ep, map_erase, cached_table_data, cache_delete_success_condition,
                                         on_cache_delete_success, on_cache_delete_failed, deleted_branch_constraints);

  Symbols symbols = TofinoModuleFactory::get_dataplane_state(ep, map_erase);

  Module *if_module                 = new If(map_erase, cache_delete_success_condition, {cache_delete_success_condition});
  Module *then_module               = new Then(map_erase);
  Module *else_module               = new Else(map_erase);
  Module *send_to_controller_module = new SendToController(on_cache_delete_failed, symbols);

  EPNode *if_node                 = new EPNode(if_module);
  EPNode *then_node               = new EPNode(then_module);
  EPNode *else_node               = new EPNode(else_module);
  EPNode *send_to_controller_node = new EPNode(send_to_controller_module);

  cached_table_delete_node->set_children(if_node);

  if_node->set_constraint(cache_delete_success_condition);
  if_node->set_prev(cached_table_delete_node);
  if_node->set_children(then_node, else_node);

  then_node->set_prev(if_node);

  else_node->set_prev(if_node);
  else_node->set_children(send_to_controller_node);

  send_to_controller_node->set_prev(else_node);

  hit_rate_t cache_success_probability = get_cache_op_success_probability(ep, map_erase, cached_table_data.key, cache_capacity);

  new_ep->get_mutable_ctx().get_mutable_profiler().insert_relative(new_ep->get_active_leaf().node->get_constraints(),
                                                                   cache_delete_success_condition, cache_success_probability);

  if (deleted_branch_constraints.has_value()) {
    new_ep->get_mutable_ctx().get_mutable_profiler().remove(deleted_branch_constraints.value());
  }

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  TofinoContext *tofino_ctx = TofinoModuleFactory::get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, map_erase, map_objs.map, cached_table);

  EPLeaf on_cache_delete_success_leaf(then_node, on_cache_delete_success);
  EPLeaf on_cache_delete_failed_leaf(send_to_controller_node, on_cache_delete_failed);

  new_ep->process_leaf(cached_table_delete_node, {on_cache_delete_success_leaf, on_cache_delete_failed_leaf});
  new_ep->replace_bdd(std::move(new_bdd));
  new_ep->assert_integrity();

  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_traffic(get_node_egress(new_ep, send_to_controller_node));

  return new_ep;
}
} // namespace

std::optional<spec_impl_t> FCFSCachedTableDeleteFactory::speculate(const EP *ep, const Node *node, const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *map_erase = dynamic_cast<const Call *>(node);
  const call_t &call    = map_erase->get_call();

  if (call.function_name != "map_erase") {
    return std::nullopt;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_map_op(ep, map_erase, map_objs)) {
    return std::nullopt;
  }

  if (!ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return std::nullopt;
  }

  fcfs_cached_table_data_t cached_table_data(ep, map_erase);

  std::vector<u32> allowed_cache_capacities = enum_fcfs_cache_cap(cached_table_data.num_entries);

  hit_rate_t chosen_cache_success_probability = 0;
  u32 chosen_cache_capacity                   = 0;
  bool successfully_placed                    = false;

  // We can use a different method for picking the right estimation depending
  // on the time it takes to find a solution.
  for (u32 cache_capacity : allowed_cache_capacities) {
    hit_rate_t cache_success_probability = get_cache_op_success_probability(ep, node, cached_table_data.key, cache_capacity);

    if (!can_get_or_build_fcfs_cached_table(ep, node, cached_table_data.obj, cached_table_data.key, cached_table_data.num_entries,
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
    return std::nullopt;
  }

  Context new_ctx          = ctx;
  const Profiler &profiler = new_ctx.get_profiler();

  hit_rate_t fraction         = profiler.get_hr(node);
  hit_rate_t on_fail_fraction = fraction * (1 - chosen_cache_success_probability);

  // FIXME: not using profiler cache.
  constraints_t constraints = node->get_ordered_branch_constraints();

  new_ctx.get_mutable_profiler().scale(constraints, chosen_cache_success_probability);
  new_ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  new_ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  new_ctx.get_mutable_perf_oracle().add_controller_traffic(on_fail_fraction);

  spec_impl_t spec_impl(decide(ep, node, {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, chosen_cache_capacity}}), new_ctx);

  std::vector<const Node *> ignore_nodes = get_future_related_nodes(ep, node, map_objs);

  for (const Node *op : ignore_nodes) {
    spec_impl.skip.insert(op->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> FCFSCachedTableDeleteFactory::process_node(const EP *ep, const Node *node,
                                                               SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *map_erase = dynamic_cast<const Call *>(node);
  const call_t &call    = map_erase->get_call();

  if (call.function_name != "map_erase") {
    return impls;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_map_op(ep, map_erase, map_objs)) {
    return impls;
  }

  if (!ep->get_ctx().can_impl_ds(map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ep->get_ctx().can_impl_ds(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return impls;
  }

  fcfs_cached_table_data_t cached_table_data(ep, map_erase);
  symbol_t cache_delete_failed              = symbol_manager->create_symbol("cache_delete_failed", 32);
  std::vector<u32> allowed_cache_capacities = enum_fcfs_cache_cap(cached_table_data.num_entries);

  for (u32 cache_capacity : allowed_cache_capacities) {
    EP *new_ep = concretize_cached_table_delete(ep, map_erase, map_objs, cached_table_data, cache_delete_failed, cache_capacity);

    if (new_ep) {
      impl_t impl = implement(ep, map_erase, new_ep, {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, cache_capacity}});
      impls.push_back(impl);
    }
  }

  return impls;
}

} // namespace tofino
} // namespace synapse