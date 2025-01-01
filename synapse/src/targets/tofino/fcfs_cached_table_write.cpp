#include "fcfs_cached_table_write.h"

#include "if.h"
#include "then.h"
#include "else.h"
#include "send_to_controller.h"

namespace tofino {

namespace {
struct fcfs_cached_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> write_value;
  u32 num_entries;

  fcfs_cached_table_data_t(const EP *ep,
                           std::vector<const Call *> future_map_puts) {
    ASSERT(!future_map_puts.empty(), "No future map puts");
    const Call *map_put = future_map_puts.front();

    const call_t &put_call = map_put->get_call();

    obj = expr_addr_to_obj_addr(put_call.args.at("map").expr);
    key = put_call.args.at("key").in;
    write_value = put_call.args.at("value").expr;
    num_entries = ep->get_ctx().get_map_config(obj).capacity;
  }
};

hit_rate_t get_cache_success_estimation_rel(const EP *ep, const Node *node,
                                            const Node *map_put,
                                            klee::ref<klee::Expr> key,
                                            u32 cache_capacity) {
  hit_rate_t fraction = ep->get_ctx().get_profiler().get_hr(node);
  hit_rate_t expected_cached_fraction =
      TofinoModuleFactory::get_fcfs_cache_success_rate(ep->get_ctx(), map_put,
                                                       key, cache_capacity);
  return fraction * expected_cached_fraction;
}

std::vector<const Node *> get_nodes_to_speculatively_ignore(
    const EP *ep, const Call *dchain_allocate_new_index,
    const map_coalescing_objs_t &map_objs, klee::ref<klee::Expr> key) {
  std::vector<const Node *> nodes_to_ignore;

  const BDD *bdd = ep->get_bdd();
  std::vector<const Call *> coalescing_nodes = get_coalescing_nodes_from_key(
      bdd, dchain_allocate_new_index, key, map_objs);

  for (const Call *coalescing_node : coalescing_nodes) {
    nodes_to_ignore.push_back(coalescing_node);
  }

  branch_direction_t index_alloc_check =
      find_branch_checking_index_alloc(ep, dchain_allocate_new_index);

  if (index_alloc_check.branch) {
    nodes_to_ignore.push_back(index_alloc_check.branch);

    const Node *next = index_alloc_check.direction
                           ? index_alloc_check.branch->get_on_false()
                           : index_alloc_check.branch->get_on_true();

    next->visit_nodes([&nodes_to_ignore](const Node *node) {
      nodes_to_ignore.push_back(node);
      return NodeVisitAction::Continue;
    });
  }

  return nodes_to_ignore;
}

klee::ref<klee::Expr>
build_cache_write_success_condition(const symbol_t &cache_write_failed) {
  klee::ref<klee::Expr> zero = solver_toolbox.exprBuilder->Constant(
      0, cache_write_failed.expr->getWidth());
  return solver_toolbox.exprBuilder->Eq(cache_write_failed.expr, zero);
}

void add_dchain_allocate_new_index_clone_on_cache_write_failed(
    const EP *ep, BDD *bdd, const Node *dchain_allocate_new_index,
    const Branch *cache_write_branch, Node *&new_on_cache_write_failed) {
  node_id_t &id = bdd->get_mutable_id();
  NodeManager &manager = bdd->get_mutable_manager();
  Node *dchain_allocate_new_index_on_cache_write_failed =
      dchain_allocate_new_index->clone(manager, false);
  dchain_allocate_new_index_on_cache_write_failed->recursive_update_ids(id);

  new_on_cache_write_failed = add_non_branch_nodes_to_bdd(
      bdd, cache_write_branch->get_on_false(),
      {dchain_allocate_new_index_on_cache_write_failed});
}

void replicate_hdr_parsing_ops_on_cache_write_failed(
    const EP *ep, BDD *bdd, const Branch *cache_write_branch,
    Node *&new_on_cache_write_failed) {
  const Node *on_cache_write_failed = cache_write_branch->get_on_false();

  std::vector<const Call *> prev_borrows = get_prev_functions(
      ep, on_cache_write_failed, {"packet_borrow_next_chunk"});

  if (prev_borrows.empty()) {
    return;
  }

  std::vector<const Node *> non_branch_nodes_to_add;
  for (const Call *prev_borrow : prev_borrows) {
    non_branch_nodes_to_add.push_back(prev_borrow);
  }

  new_on_cache_write_failed = add_non_branch_nodes_to_bdd(
      bdd, on_cache_write_failed, non_branch_nodes_to_add);
}

void delete_coalescing_nodes_on_success(
    const EP *ep, BDD *bdd, Node *on_success,
    const map_coalescing_objs_t &map_objs, klee::ref<klee::Expr> key,
    std::optional<constraints_t> &deleted_branch_constraints) {
  const std::vector<const Call *> targets =
      get_coalescing_nodes_from_key(bdd, on_success, key, map_objs);

  for (const Node *target : targets) {
    const Call *call_target = static_cast<const Call *>(target);
    const call_t &call = call_target->get_call();

    if (call.function_name == "dchain_allocate_new_index") {
      symbol_t out_of_space;
      bool found = get_symbol(call_target->get_locally_generated_symbols(),
                              "out_of_space", out_of_space);
      ASSERT(found, "Symbol out_of_space not found");

      branch_direction_t index_alloc_check =
          find_branch_checking_index_alloc(ep, on_success, out_of_space);

      if (index_alloc_check.branch) {
        ASSERT(!deleted_branch_constraints.has_value(),
               "Multiple branch checking index allocation detected");
        deleted_branch_constraints =
            index_alloc_check.branch->get_ordered_branch_constraints();

        klee::ref<klee::Expr> extra_constraint =
            index_alloc_check.branch->get_condition();

        // If we want to keep the direction on true, we must remove the on
        // false.
        if (index_alloc_check.direction) {
          extra_constraint = solver_toolbox.exprBuilder->Not(extra_constraint);
        }

        deleted_branch_constraints->push_back(extra_constraint);

        Node *trash =
            delete_branch_node_from_bdd(bdd, index_alloc_check.branch->get_id(),
                                        index_alloc_check.direction);
      }
    }

    Node *trash = delete_non_branch_node_from_bdd(bdd, target->get_id());
  }
}

std::unique_ptr<BDD> branch_bdd_on_cache_write_success(
    const EP *ep, const Node *dchain_allocate_new_index,
    const fcfs_cached_table_data_t &cached_table_data,
    klee::ref<klee::Expr> cache_write_success_condition,
    const map_coalescing_objs_t &map_objs, Node *&on_cache_write_success,
    Node *&on_cache_write_failed,
    std::optional<constraints_t> &deleted_branch_constraints) {
  const BDD *old_bdd = ep->get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const Node *next = dchain_allocate_new_index->get_next();
  ASSERT(next, "No next node");

  Branch *cache_write_branch =
      add_branch_to_bdd(new_bdd.get(), next, cache_write_success_condition);
  on_cache_write_success = cache_write_branch->get_mutable_on_true();

  add_dchain_allocate_new_index_clone_on_cache_write_failed(
      ep, new_bdd.get(), dchain_allocate_new_index, cache_write_branch,
      on_cache_write_failed);
  replicate_hdr_parsing_ops_on_cache_write_failed(
      ep, new_bdd.get(), cache_write_branch, on_cache_write_failed);

  delete_coalescing_nodes_on_success(ep, new_bdd.get(), on_cache_write_success,
                                     map_objs, cached_table_data.key,
                                     deleted_branch_constraints);

  return new_bdd;
}

EP *concretize_cached_table_write(
    const EP *ep, const Node *node, const map_coalescing_objs_t &map_objs,
    const fcfs_cached_table_data_t &cached_table_data,
    const symbol_t &cache_write_failed, u32 cache_capacity,
    const std::vector<const Call *> &future_map_puts) {
  FCFSCachedTable *cached_table =
      TofinoModuleFactory::build_or_reuse_fcfs_cached_table(
          ep, node, cached_table_data.obj, cached_table_data.key,
          cached_table_data.num_entries, cache_capacity);

  if (!cached_table) {
    return nullptr;
  }

  klee::ref<klee::Expr> cache_write_success_condition =
      build_cache_write_success_condition(cache_write_failed);

  Module *module = new FCFSCachedTableWrite(
      node, cached_table->id, cached_table_data.obj, cached_table_data.key,
      cached_table_data.write_value, cache_write_failed);
  EPNode *cached_table_write_node = new EPNode(module);

  EP *new_ep = new EP(*ep);

  Node *on_cache_write_success;
  Node *on_cache_write_failed;
  std::optional<constraints_t> deleted_branch_constraints;

  std::unique_ptr<BDD> new_bdd = branch_bdd_on_cache_write_success(
      new_ep, node, cached_table_data, cache_write_success_condition, map_objs,
      on_cache_write_success, on_cache_write_failed,
      deleted_branch_constraints);

  symbols_t symbols = TofinoModuleFactory::get_dataplane_state(ep, node);

  Module *if_module = new If(node, cache_write_success_condition,
                             {cache_write_success_condition});
  Module *then_module = new Then(node);
  Module *else_module = new Else(node);
  Module *send_to_controller_module =
      new SendToController(on_cache_write_failed, symbols);

  EPNode *if_node = new EPNode(if_module);
  EPNode *then_node = new EPNode(then_module);
  EPNode *else_node = new EPNode(else_module);
  EPNode *send_to_controller_node = new EPNode(send_to_controller_module);

  cached_table_write_node->set_children(if_node);

  if_node->set_constraint(cache_write_success_condition);
  if_node->set_prev(cached_table_write_node);
  if_node->set_children(then_node, else_node);

  then_node->set_prev(if_node);

  else_node->set_prev(if_node);
  else_node->set_children(send_to_controller_node);

  send_to_controller_node->set_prev(else_node);

  hit_rate_t cache_write_success_estimation_rel =
      get_cache_success_estimation_rel(ep, node, future_map_puts[0],
                                       cached_table_data.key, cache_capacity);

  new_ep->get_mutable_ctx().get_mutable_profiler().insert_relative(
      new_ep->get_active_leaf().node->get_constraints(),
      cache_write_success_condition, cache_write_success_estimation_rel);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  if (deleted_branch_constraints.has_value()) {
    ctx.get_mutable_profiler().remove(deleted_branch_constraints.value());
  }

  TofinoContext *tofino_ctx =
      TofinoModuleFactory::get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, node, map_objs.map, cached_table);

  EPLeaf on_cache_write_success_leaf(then_node, on_cache_write_success);
  EPLeaf on_cache_write_failed_leaf(send_to_controller_node,
                                    on_cache_write_failed);

  new_ep->process_leaf(cached_table_write_node, {on_cache_write_success_leaf,
                                                 on_cache_write_failed_leaf});
  new_ep->replace_bdd(std::move(new_bdd));
  new_ep->assert_integrity();

  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_traffic(
      get_node_egress(new_ep, send_to_controller_node));

  return new_ep;
}
} // namespace

std::optional<spec_impl_t>
FCFSCachedTableWriteFactory::speculate(const EP *ep, const Node *node,
                                       const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *dchain_allocate_new_index = static_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!is_map_update_with_dchain(ep, dchain_allocate_new_index,
                                 future_map_puts)) {
    // The cached table read should deal with these cases.
    return std::nullopt;
  }

  ASSERT(!future_map_puts.empty(), "No future map puts");

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_dchain_op(ep, dchain_allocate_new_index,
                                              map_objs)) {
    return std::nullopt;
  }

  if (!ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return std::nullopt;
  }

  fcfs_cached_table_data_t cached_table_data(ep, future_map_puts);

  std::vector<u32> allowed_cache_capacities =
      enum_fcfs_cache_cap(cached_table_data.num_entries);

  hit_rate_t chosen_success_estimation = 0;
  u32 chosen_cache_capacity = 0;
  bool successfully_placed = false;

  // We can use a different method for picking the right estimation depending
  // on the time it takes to find a solution.
  for (u32 cache_capacity : allowed_cache_capacities) {
    hit_rate_t success_estimation = get_cache_success_estimation_rel(
        ep, node, future_map_puts[0], cached_table_data.key, cache_capacity);

    if (!can_get_or_build_fcfs_cached_table(
            ep, node, cached_table_data.obj, cached_table_data.key,
            cached_table_data.num_entries, cache_capacity)) {
      break;
    }

    if (success_estimation > chosen_success_estimation) {
      chosen_success_estimation = success_estimation;
      chosen_cache_capacity = cache_capacity;
    }

    successfully_placed = true;
  }

  if (!successfully_placed) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  const Profiler &profiler = new_ctx.get_profiler();

  hit_rate_t fraction = profiler.get_hr(node);
  hit_rate_t on_fail_fraction = fraction * (1 - chosen_success_estimation);

  // FIXME: not using profiler cache.
  constraints_t constraints = node->get_ordered_branch_constraints();

  new_ctx.get_mutable_profiler().scale(constraints, chosen_success_estimation);
  new_ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  new_ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  new_ctx.get_mutable_perf_oracle().add_controller_traffic(on_fail_fraction);

  spec_impl_t spec_impl(
      decide(ep, node, {{CACHE_SIZE_PARAM, chosen_cache_capacity}}), new_ctx);

  std::vector<const Node *> ignore_nodes = get_nodes_to_speculatively_ignore(
      ep, dchain_allocate_new_index, map_objs, cached_table_data.key);

  for (const Node *op : ignore_nodes) {
    spec_impl.skip.insert(op->get_id());
  }

  return spec_impl;
}

std::vector<impl_t>
FCFSCachedTableWriteFactory::process_node(const EP *ep,
                                          const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *dchain_allocate_new_index = static_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!is_map_update_with_dchain(ep, dchain_allocate_new_index,
                                 future_map_puts)) {
    // The cached table read should deal with these cases.
    return impls;
  }

  ASSERT(!future_map_puts.empty(), "No future map puts");

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_dchain_op(ep, dchain_allocate_new_index,
                                              map_objs)) {
    return impls;
  }

  if (!ep->get_ctx().can_impl_ds(map_objs.map,
                                 DSImpl::Tofino_FCFSCachedTable) ||
      !ep->get_ctx().can_impl_ds(map_objs.dchain,
                                 DSImpl::Tofino_FCFSCachedTable)) {
    return impls;
  }

  symbol_t cache_write_failed = create_symbol("cache_write_failed", 32);

  fcfs_cached_table_data_t cached_table_data(ep, future_map_puts);

  std::vector<u32> allowed_cache_capacities =
      enum_fcfs_cache_cap(cached_table_data.num_entries);

  for (u32 cache_capacity : allowed_cache_capacities) {
    EP *new_ep = concretize_cached_table_write(
        ep, node, map_objs, cached_table_data, cache_write_failed,
        cache_capacity, future_map_puts);

    if (new_ep) {
      impl_t impl =
          implement(ep, node, new_ep, {{CACHE_SIZE_PARAM, cache_capacity}});
      impls.push_back(impl);
    }
  }

  return impls;
}

} // namespace tofino
