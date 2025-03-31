#include <LibSynapse/Modules/Tofino/FCFSCachedTableWrite.h>
#include <LibSynapse/Modules/Tofino/If.h>
#include <LibSynapse/Modules/Tofino/Then.h>
#include <LibSynapse/Modules/Tofino/Else.h>
#include <LibSynapse/Modules/Tofino/SendToController.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

namespace {
struct fcfs_cached_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> write_value;
  u32 capacity;

  fcfs_cached_table_data_t(const Context &ctx, std::vector<const LibBDD::Call *> future_map_puts) {
    assert(!future_map_puts.empty() && "No future map puts");
    const LibBDD::Call *map_put = future_map_puts.front();

    const LibBDD::call_t &put_call = map_put->get_call();

    obj         = LibCore::expr_addr_to_obj_addr(put_call.args.at("map").expr);
    key         = put_call.args.at("key").in;
    write_value = put_call.args.at("value").expr;
    capacity    = ctx.get_map_config(obj).capacity;
  }
};

hit_rate_t get_cache_success_estimation_rel(const EP *ep, const LibBDD::Node *node, const LibBDD::Node *map_put, klee::ref<klee::Expr> key,
                                            u32 cache_capacity) {
  const hit_rate_t fraction                 = ep->get_ctx().get_profiler().get_hr(node);
  const hit_rate_t expected_cached_fraction = TofinoModuleFactory::get_fcfs_cache_success_rate(ep->get_ctx(), map_put, key, cache_capacity);
  return hit_rate_t{fraction * expected_cached_fraction};
}

std::vector<const LibBDD::Node *> get_nodes_to_speculatively_ignore(const EP *ep, const LibBDD::Call *dchain_allocate_new_index,
                                                                    const LibBDD::map_coalescing_objs_t &map_objs,
                                                                    klee::ref<klee::Expr> key) {
  std::vector<const LibBDD::Call *> coalescing_nodes = dchain_allocate_new_index->get_coalescing_nodes_from_key(key, map_objs);

  std::vector<const LibBDD::Node *> nodes_to_ignore;
  for (const LibBDD::Call *coalescing_node : coalescing_nodes) {
    nodes_to_ignore.push_back(coalescing_node);
  }

  LibBDD::branch_direction_t index_alloc_check = dchain_allocate_new_index->find_branch_checking_index_alloc();

  if (index_alloc_check.branch) {
    nodes_to_ignore.push_back(index_alloc_check.branch);

    const LibBDD::Node *next =
        index_alloc_check.direction ? index_alloc_check.branch->get_on_false() : index_alloc_check.branch->get_on_true();

    next->visit_nodes([&nodes_to_ignore](const LibBDD::Node *node) {
      nodes_to_ignore.push_back(node);
      return LibBDD::NodeVisitAction::Continue;
    });
  }

  return nodes_to_ignore;
}

klee::ref<klee::Expr> build_cache_write_success_condition(const LibCore::symbol_t &cache_write_failed) {
  klee::ref<klee::Expr> zero = LibCore::solver_toolbox.exprBuilder->Constant(0, cache_write_failed.expr->getWidth());
  return LibCore::solver_toolbox.exprBuilder->Eq(cache_write_failed.expr, zero);
}

void add_dchain_allocate_new_index_clone_on_cache_write_failed(const EP *ep, LibBDD::BDD *bdd,
                                                               const LibBDD::Node *dchain_allocate_new_index,
                                                               const LibBDD::Branch *cache_write_branch,
                                                               LibBDD::Node *&new_on_cache_write_failed) {
  LibBDD::node_id_t &id                                         = bdd->get_mutable_id();
  LibBDD::NodeManager &manager                                  = bdd->get_mutable_manager();
  LibBDD::Node *dchain_allocate_new_index_on_cache_write_failed = dchain_allocate_new_index->clone(manager, false);
  dchain_allocate_new_index_on_cache_write_failed->recursive_update_ids(id);

  new_on_cache_write_failed =
      bdd->clone_and_add_non_branches(cache_write_branch->get_on_false(), {dchain_allocate_new_index_on_cache_write_failed});
}

void replicate_hdr_parsing_ops_on_cache_write_failed(const EP *ep, LibBDD::BDD *bdd, const LibBDD::Branch *cache_write_branch,
                                                     LibBDD::Node *&new_on_cache_write_failed) {
  const LibBDD::Node *on_cache_write_failed = cache_write_branch->get_on_false();

  std::vector<const LibBDD::Call *> prev_borrows =
      on_cache_write_failed->get_prev_functions({"packet_borrow_next_chunk"}, ep->get_target_roots(ep->get_active_target()));

  if (prev_borrows.empty()) {
    return;
  }

  std::vector<const LibBDD::Node *> non_branch_nodes_to_add;
  for (const LibBDD::Call *prev_borrow : prev_borrows) {
    non_branch_nodes_to_add.push_back(prev_borrow);
  }

  new_on_cache_write_failed = bdd->clone_and_add_non_branches(on_cache_write_failed, non_branch_nodes_to_add);
}

void delete_coalescing_nodes_on_success(const EP *ep, LibBDD::BDD *bdd, LibBDD::Node *on_success,
                                        const LibBDD::map_coalescing_objs_t &map_objs, klee::ref<klee::Expr> key,
                                        std::optional<std::vector<klee::ref<klee::Expr>>> &deleted_branch_constraints) {
  const std::vector<const LibBDD::Call *> targets = on_success->get_coalescing_nodes_from_key(key, map_objs);

  for (const LibBDD::Node *target : targets) {
    const LibBDD::Call *call_target = dynamic_cast<const LibBDD::Call *>(target);
    const LibBDD::call_t &call      = call_target->get_call();

    if (call.function_name == "dchain_allocate_new_index") {
      LibBDD::branch_direction_t index_alloc_check = call_target->find_branch_checking_index_alloc();

      if (index_alloc_check.branch) {
        assert(!deleted_branch_constraints.has_value() && "Multiple branch checking index allocation detected");
        deleted_branch_constraints = index_alloc_check.branch->get_ordered_branch_constraints();

        klee::ref<klee::Expr> extra_constraint = index_alloc_check.branch->get_condition();

        // If we want to keep the direction on true, we must remove the on
        // false.
        if (index_alloc_check.direction) {
          extra_constraint = LibCore::solver_toolbox.exprBuilder->Not(extra_constraint);
        }

        deleted_branch_constraints->push_back(extra_constraint);

        bdd->delete_branch(index_alloc_check.branch->get_id(), index_alloc_check.direction);
      }
    }

    bdd->delete_non_branch(target->get_id());
  }
}

std::unique_ptr<LibBDD::BDD>
branch_bdd_on_cache_write_success(const EP *ep, const LibBDD::Node *dchain_allocate_new_index,
                                  const fcfs_cached_table_data_t &cached_table_data, klee::ref<klee::Expr> cache_write_success_condition,
                                  const LibBDD::map_coalescing_objs_t &map_objs, LibBDD::Node *&on_cache_write_success,
                                  LibBDD::Node *&on_cache_write_failed,
                                  std::optional<std::vector<klee::ref<klee::Expr>>> &deleted_branch_constraints) {
  const LibBDD::BDD *old_bdd           = ep->get_bdd();
  std::unique_ptr<LibBDD::BDD> new_bdd = std::make_unique<LibBDD::BDD>(*old_bdd);

  const LibBDD::Node *next = dchain_allocate_new_index->get_next();
  assert(next && "No next node");

  LibBDD::Branch *cache_write_branch = new_bdd->clone_and_add_branch(next, cache_write_success_condition);
  on_cache_write_success             = cache_write_branch->get_mutable_on_true();

  add_dchain_allocate_new_index_clone_on_cache_write_failed(ep, new_bdd.get(), dchain_allocate_new_index, cache_write_branch,
                                                            on_cache_write_failed);
  replicate_hdr_parsing_ops_on_cache_write_failed(ep, new_bdd.get(), cache_write_branch, on_cache_write_failed);

  delete_coalescing_nodes_on_success(ep, new_bdd.get(), on_cache_write_success, map_objs, cached_table_data.key,
                                     deleted_branch_constraints);

  return new_bdd;
}

EP *concretize_cached_table_write(const EP *ep, const LibBDD::Node *node, const LibBDD::map_coalescing_objs_t &map_objs,
                                  const fcfs_cached_table_data_t &cached_table_data, const LibCore::symbol_t &cache_write_failed,
                                  u32 cache_capacity, const std::vector<const LibBDD::Call *> &future_map_puts) {
  FCFSCachedTable *cached_table = TofinoModuleFactory::build_or_reuse_fcfs_cached_table(
      ep, node, cached_table_data.obj, cached_table_data.key, cached_table_data.capacity, cache_capacity);

  if (!cached_table) {
    return nullptr;
  }

  klee::ref<klee::Expr> cache_write_success_condition = build_cache_write_success_condition(cache_write_failed);

  Module *module                  = new FCFSCachedTableWrite(node, cached_table->id, cached_table_data.obj, cached_table_data.key,
                                                             cached_table_data.write_value, cache_write_failed);
  EPNode *cached_table_write_node = new EPNode(module);

  EP *new_ep = new EP(*ep);

  LibBDD::Node *on_cache_write_success;
  LibBDD::Node *on_cache_write_failed;
  std::optional<std::vector<klee::ref<klee::Expr>>> deleted_branch_constraints;

  std::unique_ptr<LibBDD::BDD> new_bdd =
      branch_bdd_on_cache_write_success(new_ep, node, cached_table_data, cache_write_success_condition, map_objs, on_cache_write_success,
                                        on_cache_write_failed, deleted_branch_constraints);

  LibCore::Symbols symbols = TofinoModuleFactory::get_relevant_dataplane_state(ep, node);

  Module *if_module                 = new If(node, cache_write_success_condition, {cache_write_success_condition});
  Module *then_module               = new Then(node);
  Module *else_module               = new Else(node);
  Module *send_to_controller_module = new SendToController(on_cache_write_failed, symbols);

  EPNode *if_node                 = new EPNode(if_module);
  EPNode *then_node               = new EPNode(then_module);
  EPNode *else_node               = new EPNode(else_module);
  EPNode *send_to_controller_node = new EPNode(send_to_controller_module);

  cached_table_write_node->set_children(if_node);

  if_node->set_constraint(cache_write_success_condition);
  if_node->set_prev(cached_table_write_node);
  if_node->set_children(then_node, else_node);

  then_node->set_prev(if_node);

  else_node->set_prev(if_node);
  else_node->set_children(send_to_controller_node);

  send_to_controller_node->set_prev(else_node);

  const hit_rate_t cache_write_success_estimation_rel =
      get_cache_success_estimation_rel(ep, node, future_map_puts[0], cached_table_data.key, cache_capacity);

  new_ep->get_mutable_ctx().get_mutable_profiler().insert_relative(new_ep->get_active_leaf().node->get_constraints(),
                                                                   cache_write_success_condition, cache_write_success_estimation_rel);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  if (deleted_branch_constraints.has_value()) {
    ctx.get_mutable_profiler().remove(deleted_branch_constraints.value());
  }

  TofinoContext *tofino_ctx = TofinoModuleFactory::get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, node, map_objs.map, cached_table);

  EPLeaf on_cache_write_success_leaf(then_node, on_cache_write_success);
  EPLeaf on_cache_write_failed_leaf(send_to_controller_node, on_cache_write_failed);

  new_ep->process_leaf(cached_table_write_node, {on_cache_write_success_leaf, on_cache_write_failed_leaf});
  new_ep->replace_bdd(std::move(new_bdd));
  new_ep->assert_integrity();

  const hit_rate_t hr = new_ep->get_ctx().get_profiler().get_hr(send_to_controller_node);
  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_traffic(new_ep->get_node_egress(hr, send_to_controller_node));

  return new_ep;
}
} // namespace

std::optional<spec_impl_t> FCFSCachedTableWriteFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *dchain_allocate_new_index = dynamic_cast<const LibBDD::Call *>(node);

  std::vector<const LibBDD::Call *> future_map_puts;
  if (!ep->get_bdd()->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts)) {
    // The cached table read should deal with these cases.
    return std::nullopt;
  }

  assert(!future_map_puts.empty() && "No future map puts");

  LibBDD::map_coalescing_objs_t map_objs;
  if (!ep->get_bdd()->get_map_coalescing_objs_from_dchain_op(dchain_allocate_new_index, map_objs)) {
    return std::nullopt;
  }

  if (!ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_FCFSCachedTable) || !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return std::nullopt;
  }

  fcfs_cached_table_data_t cached_table_data(ep->get_ctx(), future_map_puts);

  std::vector<u32> allowed_cache_capacities = enum_fcfs_cache_cap(cached_table_data.capacity);

  hit_rate_t chosen_success_estimation = 0_hr;
  u32 chosen_cache_capacity            = 0;
  bool successfully_placed             = false;

  // We can use a different method for picking the right estimation depending
  // on the time it takes to find a solution.
  for (u32 cache_capacity : allowed_cache_capacities) {
    const hit_rate_t success_estimation =
        get_cache_success_estimation_rel(ep, node, future_map_puts[0], cached_table_data.key, cache_capacity);

    if (!can_get_or_build_fcfs_cached_table(ep, node, cached_table_data.obj, cached_table_data.key, cached_table_data.capacity,
                                            cache_capacity)) {
      break;
    }

    if (success_estimation > chosen_success_estimation) {
      chosen_success_estimation = success_estimation;
      chosen_cache_capacity     = cache_capacity;
    }

    successfully_placed = true;
  }

  if (!successfully_placed) {
    return std::nullopt;
  }

  Context new_ctx          = ctx;
  const Profiler &profiler = new_ctx.get_profiler();

  const hit_rate_t fraction         = profiler.get_hr(node);
  const hit_rate_t on_fail_fraction = hit_rate_t{fraction * (1 - chosen_success_estimation)};

  // FIXME: not using profiler cache.
  std::vector<klee::ref<klee::Expr>> constraints = node->get_ordered_branch_constraints();

  new_ctx.get_mutable_profiler().scale(constraints, chosen_success_estimation.value);
  new_ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  new_ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  new_ctx.get_mutable_perf_oracle().add_controller_traffic(on_fail_fraction);

  spec_impl_t spec_impl(decide(ep, node, {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, chosen_cache_capacity}}), new_ctx);

  std::vector<const LibBDD::Node *> ignore_nodes =
      get_nodes_to_speculatively_ignore(ep, dchain_allocate_new_index, map_objs, cached_table_data.key);

  for (const LibBDD::Node *op : ignore_nodes) {
    spec_impl.skip.insert(op->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> FCFSCachedTableWriteFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                              LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *dchain_allocate_new_index = dynamic_cast<const LibBDD::Call *>(node);

  std::vector<const LibBDD::Call *> future_map_puts;
  if (!ep->get_bdd()->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts)) {
    // The cached table read should deal with these cases.
    return impls;
  }

  assert(!future_map_puts.empty() && "No future map puts");

  LibBDD::map_coalescing_objs_t map_objs;
  if (!ep->get_bdd()->get_map_coalescing_objs_from_dchain_op(dchain_allocate_new_index, map_objs)) {
    return impls;
  }

  if (!ep->get_ctx().can_impl_ds(map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ep->get_ctx().can_impl_ds(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return impls;
  }

  LibCore::symbol_t cache_write_failed = symbol_manager->create_symbol("cache_write_failed", 32);
  fcfs_cached_table_data_t cached_table_data(ep->get_ctx(), future_map_puts);
  std::vector<u32> allowed_cache_capacities = enum_fcfs_cache_cap(cached_table_data.capacity);

  for (u32 cache_capacity : allowed_cache_capacities) {
    EP *new_ep = concretize_cached_table_write(ep, node, map_objs, cached_table_data, cache_write_failed, cache_capacity, future_map_puts);

    if (new_ep) {
      impl_t impl = implement(ep, node, new_ep, {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, cache_capacity}});
      impls.push_back(impl);
    }
  }

  return impls;
}

std::unique_ptr<Module> FCFSCachedTableWriteFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *dchain_allocate_new_index = dynamic_cast<const LibBDD::Call *>(node);

  std::vector<const LibBDD::Call *> future_map_puts;
  if (!bdd->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts)) {
    return {};
  }

  assert(!future_map_puts.empty() && "No future map puts");

  LibBDD::map_coalescing_objs_t map_objs;
  if (!bdd->get_map_coalescing_objs_from_dchain_op(dchain_allocate_new_index, map_objs)) {
    return {};
  }

  if (!ctx.check_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ctx.check_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  fcfs_cached_table_data_t cached_table_data(ctx, future_map_puts);
  LibCore::symbol_t mock_cache_write_failed;

  const std::unordered_set<LibSynapse::Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_ds(map_objs.map);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const FCFSCachedTable *fcfs_cached_table = dynamic_cast<const FCFSCachedTable *>(*ds.begin());

  return std::make_unique<FCFSCachedTableWrite>(node, fcfs_cached_table->id, cached_table_data.obj, cached_table_data.key,
                                                cached_table_data.write_value, mock_cache_write_failed);
}

} // namespace Tofino
} // namespace LibSynapse