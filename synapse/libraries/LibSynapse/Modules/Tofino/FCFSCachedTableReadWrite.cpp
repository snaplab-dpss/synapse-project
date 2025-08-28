#include <LibSynapse/Modules/Tofino/FCFSCachedTableReadWrite.h>
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
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;
  symbol_t map_has_this_key;
  u32 capacity;

  fcfs_cached_table_data_t(const Context &ctx, const Call *map_get, std::vector<const Call *> future_map_puts) {
    assert(!future_map_puts.empty() && "No future map puts found");
    const Call *map_put = future_map_puts.front();

    const call_t &get_call = map_get->get_call();
    const call_t &put_call = map_put->get_call();

    obj              = expr_addr_to_obj_addr(get_call.args.at("map").expr);
    key              = get_call.args.at("key").in;
    read_value       = get_call.args.at("value_out").out;
    write_value      = put_call.args.at("value").expr;
    map_has_this_key = map_get->get_local_symbol("map_has_this_key");
    capacity         = ctx.get_map_config(obj).capacity;
  }
};

klee::ref<klee::Expr> build_cache_write_success_condition(const symbol_t &cache_write_failed) {
  bits_t width               = cache_write_failed.expr->getWidth();
  klee::ref<klee::Expr> zero = solver_toolbox.exprBuilder->Constant(0, width);
  return solver_toolbox.exprBuilder->Eq(cache_write_failed.expr, zero);
}

hit_rate_t get_cache_op_success_probability(const EP *ep, const BDDNode *node, klee::ref<klee::Expr> key, u32 cache_capacity) {
  assert(node->get_type() == BDDNodeType::Call && "Unexpected node type");
  const Call *map_get = dynamic_cast<const Call *>(node);

  const hit_rate_t hr        = ep->get_ctx().get_profiler().get_hr(map_get);
  const rw_fractions_t rw_hr = ep->get_ctx().get_profiler().get_cond_map_put_rw_profile_fractions(map_get);

  const hit_rate_t rp = hit_rate_t{rw_hr.read / hr};
  const hit_rate_t wp = hit_rate_t{rw_hr.write / hr};
  // hit_rate_t failed_writes = rw_hr.write_attempt - rw_hr.write;

  const hit_rate_t cache_hit_rate = TofinoModuleFactory::get_fcfs_cache_success_rate(ep->get_ctx(), node, key, cache_capacity);
  const hit_rate_t probability    = rp + wp * cache_hit_rate;

  return probability;
}

void add_map_get_clone_on_cache_write_failed(const EP *ep, BDD *bdd, const BDDNode *map_get, const Branch *cache_write_branch,
                                             BDDNode *&new_on_cache_write_failed) {
  bdd_node_id_t &id                      = bdd->get_mutable_id();
  BDDNodeManager &manager                = bdd->get_mutable_manager();
  BDDNode *map_get_on_cache_write_failed = map_get->clone(manager, false);
  map_get_on_cache_write_failed->recursive_update_ids(id);

  new_on_cache_write_failed = bdd->add_cloned_non_branches(cache_write_branch->get_on_false()->get_id(), {map_get_on_cache_write_failed});
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

std::vector<const BDDNode *> get_nodes_to_speculatively_ignore(const EP *ep, const BDDNode *on_success, const map_coalescing_objs_t &map_objs,
                                                               klee::ref<klee::Expr> key) {
  const std::vector<const Call *> coalescing_nodes = on_success->get_coalescing_nodes_from_key(key, map_objs);

  std::vector<const BDDNode *> nodes_to_ignore;
  for (const Call *coalescing_node : coalescing_nodes) {
    nodes_to_ignore.push_back(coalescing_node);

    const call_t &call = coalescing_node->get_call();

    if (call.function_name == "dchain_allocate_new_index") {
      const branch_direction_t index_alloc_check = ep->get_bdd()->find_branch_checking_index_alloc(coalescing_node);

      // FIXME: We ignore all logic happening when the index is not
      // successfuly allocated. This is a major simplification, as the NF
      // might be doing something useful here. We never encountered a scenario
      // where this was the case, but it could happen nonetheless.
      if (index_alloc_check.branch) {
        nodes_to_ignore.push_back(index_alloc_check.branch);

        const BDDNode *next = index_alloc_check.direction ? index_alloc_check.branch->get_on_false() : index_alloc_check.branch->get_on_true();

        next->visit_nodes([&nodes_to_ignore](const BDDNode *node) {
          nodes_to_ignore.push_back(node);
          return BDDNodeVisitAction::Continue;
        });
      }
    }
  }

  return nodes_to_ignore;
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

std::unique_ptr<BDD> branch_bdd_on_cache_write_success(const EP *ep, const BDDNode *map_get, const fcfs_cached_table_data_t &fcfs_cached_table_data,
                                                       klee::ref<klee::Expr> cache_write_success_condition, const map_coalescing_objs_t &map_objs,
                                                       BDDNode *&on_cache_write_success, BDDNode *&on_cache_write_failed,
                                                       std::optional<std::vector<klee::ref<klee::Expr>>> &deleted_branch_constraints) {
  const BDD *old_bdd           = ep->get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const BDDNode *next = map_get->get_next();
  assert(next && "map_get node has no next node");

  Branch *cache_write_branch = new_bdd->add_cloned_branch(next->get_id(), cache_write_success_condition);

  on_cache_write_success = cache_write_branch->get_mutable_on_true();
  add_map_get_clone_on_cache_write_failed(ep, new_bdd.get(), map_get, cache_write_branch, on_cache_write_failed);

  replicate_hdr_parsing_ops_on_cache_write_failed(ep, new_bdd.get(), cache_write_branch, on_cache_write_failed);

  delete_coalescing_nodes_on_success(ep, new_bdd.get(), on_cache_write_success, map_objs, fcfs_cached_table_data.key, deleted_branch_constraints);

  return new_bdd;
}

std::unique_ptr<EP> concretize_cached_table_cond_write(const EP *ep, const BDDNode *node, const map_coalescing_objs_t &map_objs,
                                                       const fcfs_cached_table_data_t &fcfs_cached_table_data, const symbol_t &cache_write_failed,
                                                       u32 cache_capacity) {
  FCFSCachedTable *cached_table = TofinoModuleFactory::build_or_reuse_fcfs_cached_table(
      ep, node, fcfs_cached_table_data.obj, fcfs_cached_table_data.key, fcfs_cached_table_data.capacity, cache_capacity);

  if (!cached_table) {
    return nullptr;
  }

  klee::ref<klee::Expr> cache_write_success_condition = build_cache_write_success_condition(cache_write_failed);

  Module *module = new FCFSCachedTableReadWrite(node, cached_table->id, cached_table->tables.back().id, fcfs_cached_table_data.obj,
                                                fcfs_cached_table_data.key, fcfs_cached_table_data.read_value, fcfs_cached_table_data.write_value,
                                                fcfs_cached_table_data.map_has_this_key, cache_write_failed);
  EPNode *cached_table_cond_write_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  BDDNode *on_cache_write_success;
  BDDNode *on_cache_write_failed;
  std::optional<std::vector<klee::ref<klee::Expr>>> deleted_branch_constraints;

  panic("TODO: Remove vector key operations (and do the same for the other data structures)");
  std::unique_ptr<BDD> new_bdd =
      branch_bdd_on_cache_write_success(new_ep.get(), node, fcfs_cached_table_data, cache_write_success_condition, map_objs, on_cache_write_success,
                                        on_cache_write_failed, deleted_branch_constraints);

  Symbols symbols = TofinoModuleFactory::get_relevant_dataplane_state(ep, node);

  Module *if_module                 = new If(node, cache_write_success_condition, {cache_write_success_condition});
  Module *then_module               = new Then(node);
  Module *else_module               = new Else(node);
  Module *send_to_controller_module = new SendToController(on_cache_write_failed, symbols);

  EPNode *if_node                 = new EPNode(if_module);
  EPNode *then_node               = new EPNode(then_module);
  EPNode *else_node               = new EPNode(else_module);
  EPNode *send_to_controller_node = new EPNode(send_to_controller_module);

  cached_table_cond_write_node->set_children(if_node);

  if_node->set_prev(cached_table_cond_write_node);
  if_node->set_children(cache_write_success_condition, then_node, else_node);

  then_node->set_prev(if_node);

  else_node->set_prev(if_node);
  else_node->set_children(send_to_controller_node);

  send_to_controller_node->set_prev(else_node);

  const hit_rate_t cache_success_probability = get_cache_op_success_probability(ep, node, fcfs_cached_table_data.key, cache_capacity);

  new_ep->get_mutable_ctx().get_mutable_profiler().insert_relative(new_ep->get_active_leaf().node->get_constraints(), cache_write_success_condition,
                                                                   cache_success_probability);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  if (deleted_branch_constraints.has_value()) {
    ctx.get_mutable_profiler().remove(deleted_branch_constraints.value());
  }

  TofinoContext *tofino_ctx = TofinoModuleFactory::get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, map_objs.map, cached_table);

  EPLeaf on_cache_write_success_leaf(then_node, on_cache_write_success);
  EPLeaf on_cache_write_failed_leaf(send_to_controller_node, on_cache_write_failed);

  new_ep->process_leaf(cached_table_cond_write_node, {on_cache_write_success_leaf, on_cache_write_failed_leaf});
  new_ep->replace_bdd(std::move(new_bdd));

  const hit_rate_t hr = new_ep->get_ctx().get_profiler().get_hr(send_to_controller_node);
  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_traffic(new_ep->get_node_egress(hr, send_to_controller_node));

  return new_ep;
}
} // namespace

std::optional<spec_impl_t> FCFSCachedTableReadWriteFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!ep->get_bdd()->is_map_get_followed_by_map_puts_on_miss(map_get, future_map_puts)) {
    // The cached table read should deal with these cases.
    return {};
  }

  const fcfs_cached_table_data_t cached_table_data(ep->get_ctx(), map_get, future_map_puts);

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(cached_table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ctx.can_impl_ds(map_objs->map, DSImpl::Tofino_FCFSCachedTable) || !ctx.can_impl_ds(map_objs->dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const std::vector<u32> allowed_cache_capacities = enum_fcfs_cache_cap(cached_table_data.capacity);

  hit_rate_t chosen_success_probability = 0_hr;
  u32 chosen_cache_capacity             = 0;
  bool successfully_placed              = false;

  // We can use a different method for picking the right estimation depending
  // on the time it takes to find a solution.
  for (u32 cache_capacity : allowed_cache_capacities) {
    const hit_rate_t success_probability = get_cache_op_success_probability(ep, node, cached_table_data.key, cache_capacity);

    if (!can_get_or_build_fcfs_cached_table(ep, node, cached_table_data.obj, cached_table_data.key, cached_table_data.capacity, cache_capacity)) {
      break;
    }

    if (success_probability > chosen_success_probability) {
      chosen_success_probability = success_probability;
      chosen_cache_capacity      = cache_capacity;
    }

    successfully_placed = true;
  }

  if (!successfully_placed) {
    return {};
  }

  Context new_ctx = ctx;

  const hit_rate_t hr         = new_ctx.get_profiler().get_hr(node);
  const hit_rate_t on_fail_hr = hit_rate_t{hr * (1 - chosen_success_probability)};

  // FIXME: not using profiler cache.
  std::vector<klee::ref<klee::Expr>> constraints = node->get_ordered_branch_constraints();
  new_ctx.get_mutable_profiler().scale(constraints, chosen_success_probability.value);
  new_ctx.save_ds_impl(map_objs->map, DSImpl::Tofino_FCFSCachedTable);
  new_ctx.save_ds_impl(map_objs->dchain, DSImpl::Tofino_FCFSCachedTable);

  new_ctx.get_mutable_perf_oracle().add_controller_traffic(on_fail_hr);

  spec_impl_t spec_impl(decide(ep, node, {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, chosen_cache_capacity}}), new_ctx);

  std::vector<const BDDNode *> ignore_nodes = get_nodes_to_speculatively_ignore(ep, map_get, map_objs.value(), cached_table_data.key);
  for (const BDDNode *op : ignore_nodes) {
    spec_impl.skip.insert(op->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> FCFSCachedTableReadWriteFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;

  if (!ep->get_bdd()->is_map_get_followed_by_map_puts_on_miss(map_get, future_map_puts)) {
    // The cached table read should deal with these cases.
    return {};
  }

  const fcfs_cached_table_data_t cached_table_data(ep->get_ctx(), map_get, future_map_puts);

  const std::optional<map_coalescing_objs_t> map_objs = ep->get_ctx().get_map_coalescing_objs(cached_table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(map_objs->map, DSImpl::Tofino_FCFSCachedTable) ||
      !ep->get_ctx().can_impl_ds(map_objs->dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const symbol_t cache_write_failed               = symbol_manager->create_symbol("cache_write_failed", 32);
  const std::vector<u32> allowed_cache_capacities = enum_fcfs_cache_cap(cached_table_data.capacity);

  std::vector<impl_t> impls;
  for (u32 cache_capacity : allowed_cache_capacities) {
    std::unique_ptr<EP> new_ep =
        concretize_cached_table_cond_write(ep, node, map_objs.value(), cached_table_data, cache_write_failed, cache_capacity);
    if (new_ep) {
      impl_t impl = implement(ep, node, std::move(new_ep), {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, cache_capacity}});
      impls.push_back(std::move(impl));
    }
  }

  return impls;
}

std::unique_ptr<Module> FCFSCachedTableReadWriteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!bdd->is_map_get_followed_by_map_puts_on_miss(map_get, future_map_puts)) {
    return {};
  }

  const fcfs_cached_table_data_t cached_table_data(ctx, map_get, future_map_puts);

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(cached_table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_FCFSCachedTable) || !ctx.check_ds_impl(map_objs->dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  symbol_t mock_cache_write_failed;

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(map_objs->map);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const FCFSCachedTable *fcfs_cached_table = dynamic_cast<const FCFSCachedTable *>(*ds.begin());

  return std::make_unique<FCFSCachedTableReadWrite>(node, fcfs_cached_table->id, fcfs_cached_table->tables.back().id, cached_table_data.obj,
                                                    cached_table_data.key, cached_table_data.read_value, cached_table_data.write_value,
                                                    cached_table_data.map_has_this_key, mock_cache_write_failed);
}

} // namespace Tofino
} // namespace LibSynapse