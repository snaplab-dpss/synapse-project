#include <LibSynapse/Modules/Tofino/FCFSCachedTableReadWrite.h>
#include <LibSynapse/Modules/Tofino/If.h>
#include <LibSynapse/Modules/Tofino/Then.h>
#include <LibSynapse/Modules/Tofino/Else.h>
#include <LibSynapse/Modules/Tofino/SendToController.h>
#include <LibSynapse/Modules/Controller/DataplaneFCFSCachedTableWrite.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibBDD/BDD.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::BDDNodeManager;
using LibBDD::Branch;
using LibBDD::branch_direction_t;
using LibBDD::Call;
using LibBDD::call_t;
using LibBDD::Route;

using LibSynapse::Controller::DataplaneFCFSCachedTableWrite;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct fcfs_cached_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> original_key;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;
  symbol_t map_has_this_key;
  u32 capacity;
  map_coalescing_objs_t map_objs;
};

std::optional<fcfs_cached_table_data_t> build_fcfs_cached_table_data(const BDD *bdd, const Context &ctx, const Call *map_get, const Call *map_put) {
  const call_t &get_call = map_get->get_call();
  const call_t &put_call = map_put->get_call();

  fcfs_cached_table_data_t data;
  data.obj              = expr_addr_to_obj_addr(get_call.args.at("map").expr);
  data.original_key     = get_call.args.at("key").in;
  data.keys             = Table::build_keys(data.original_key, ctx.get_expr_structs());
  data.read_value       = get_call.args.at("value_out").out;
  data.write_value      = put_call.args.at("value").expr;
  data.map_has_this_key = map_get->get_local_symbol("map_has_this_key");
  data.capacity         = ctx.get_map_config(data.obj).capacity;

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(data.obj);
  if (!map_objs.has_value()) {
    return {};
  }
  data.map_objs = map_objs.value();

  return data;
}

struct read_write_pattern_t {
  const Call *map_get;
  const Call *map_put;
  const Call *dchain_allocate_new_index;
  const BDDNode *on_read_success;
  const BDDNode *on_read_failure;
  const BDDNode *on_write_success;
  const BDDNode *on_write_failure;
};

bool is_read_write_pattern(const BDD *bdd, const BDDNode *node, read_write_pattern_t &pattern) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *map_get = dynamic_cast<const Call *>(node);

  branch_direction_t map_get_success_direction;
  if (!bdd->is_map_get_and_branch_checking_success(map_get, map_get->get_next(), map_get_success_direction)) {
    return false;
  }

  const BDDNode *on_read_success = map_get_success_direction.get_success_node();
  const BDDNode *on_read_failure = map_get_success_direction.get_failure_node();

  if (on_read_failure->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(on_read_failure);
  if (!bdd->is_index_alloc_on_unsuccessful_map_get(dchain_allocate_new_index)) {
    return false;
  }

  const branch_direction_t index_alloc_success_direction = bdd->find_branch_checking_index_alloc(dchain_allocate_new_index);
  if (index_alloc_success_direction.branch == nullptr) {
    return false;
  }

  const BDDNode *on_index_allocation_success = index_alloc_success_direction.get_success_node();
  const BDDNode *on_index_allocation_failure = index_alloc_success_direction.get_failure_node();

  if (on_index_allocation_success->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *map_put = dynamic_cast<const Call *>(on_index_allocation_success);
  std::vector<const Call *> future_map_puts;
  if (!bdd->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts) || future_map_puts.size() != 1 || future_map_puts[0] != map_put) {
    return false;
  }

  const BDDNode *on_write_success = map_put->get_next();

  pattern.map_get                   = map_get;
  pattern.map_put                   = map_put;
  pattern.dchain_allocate_new_index = dchain_allocate_new_index;
  pattern.on_read_success           = on_read_success;
  pattern.on_read_failure           = on_read_failure;
  pattern.on_write_success          = on_write_success;
  pattern.on_write_failure          = on_index_allocation_failure;

  return true;
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

// void add_map_get_clone_on_cache_miss(BDD *bdd, const BDDNode *map_get, const Branch *cache_hit_branch, BDDNode *&new_on_cache_miss) {
//   bdd_node_id_t &id              = bdd->get_mutable_id();
//   BDDNodeManager &manager        = bdd->get_mutable_manager();
//   BDDNode *map_get_on_cache_miss = map_get->clone(manager, false);
//   map_get_on_cache_miss->recursive_update_ids(id);
//   new_on_cache_miss = bdd->add_cloned_non_branches(cache_hit_branch->get_on_false()->get_id(), {map_get_on_cache_miss});
// }

// void replicate_hdr_parsing_ops_on_cache_miss(const EP *ep, BDD *bdd, const Branch *cache_hit_branch, BDDNode *&new_on_cache_miss) {
//   const BDDNode *on_cache_miss = cache_hit_branch->get_on_false();

//   std::list<const Call *> prev_borrows =
//       on_cache_miss->get_prev_functions({"packet_borrow_next_chunk"}, ep->get_target_roots(ep->get_active_target()));

//   if (prev_borrows.empty()) {
//     return;
//   }

//   std::vector<const BDDNode *> non_branch_nodes_to_add;
//   for (const Call *prev_borrow : prev_borrows) {
//     non_branch_nodes_to_add.push_back(prev_borrow);
//   }

//   new_on_cache_miss = bdd->add_cloned_non_branches(on_cache_miss->get_id(), non_branch_nodes_to_add);
// }

std::vector<const BDDNode *> get_nodes_to_speculatively_ignore(const EP *ep, const BDDNode *node, const map_coalescing_objs_t &map_objs,
                                                               klee::ref<klee::Expr> key) {
  const std::vector<const Call *> coalescing_nodes = node->get_coalescing_nodes_from_key(key, map_objs);

  std::vector<const BDDNode *> nodes_to_ignore;
  for (const Call *coalescing_node : coalescing_nodes) {
    if (coalescing_node == node) {
      continue;
    }

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

        next->visit_nodes([&nodes_to_ignore](const BDDNode *future_node) {
          nodes_to_ignore.push_back(future_node);
          return BDDNodeVisitAction::Continue;
        });
      }
    }
  }

  return nodes_to_ignore;
}

// void delete_coalescing_nodes(const EP *ep, BDD *bdd, BDDNode *on_success, const map_coalescing_objs_t &map_objs, klee::ref<klee::Expr> key,
//                              std::vector<klee::ref<klee::Expr>> &deleted_branch_constraints) {
//   const std::vector<const Call *> targets = on_success->get_coalescing_nodes_from_key(key, map_objs);

//   for (const BDDNode *target : targets) {
//     const Call *call_target = dynamic_cast<const Call *>(target);
//     const call_t &call      = call_target->get_call();

//     if (call.function_name == "dchain_allocate_new_index") {
//       const branch_direction_t index_alloc_check = bdd->find_branch_checking_index_alloc(call_target);

//       if (index_alloc_check.branch) {
//         assert(deleted_branch_constraints.empty() && "Multiple branch checking index allocation detected");
//         deleted_branch_constraints = index_alloc_check.branch->get_ordered_branch_constraints();

//         klee::ref<klee::Expr> extra_constraint = index_alloc_check.branch->get_condition();

//         // If we want to keep the direction on true, we must remove the on false.
//         if (index_alloc_check.direction) {
//           extra_constraint = solver_toolbox.exprBuilder->Not(extra_constraint);
//         }

//         deleted_branch_constraints.push_back(extra_constraint);

//         const BDD::BranchDeletionAction branch_deletion_action =
//             index_alloc_check.direction ? BDD::BranchDeletionAction::KeepOnTrue : BDD::BranchDeletionAction::KeepOnFalse;
//         bdd->delete_branch(index_alloc_check.branch->get_id(), branch_deletion_action);
//       }
//     }

//     bdd->delete_non_branch(target->get_id());
//   }
// }

struct rebuilt_bdd_result_t {
  std::unique_ptr<BDD> bdd;

  // For when we require the control plane to perform the write operation.
  BDDNode *on_collision_detected;

  // For when the key is not found but there are no more available indices, and the write operation fails.
  BDDNode *on_index_allocation_failed;

  // For when the data plane fails to find the key but successfully allocates a new index and performs the write.
  BDDNode *data_plane_write_success;

  // For when the data plane successfully finds the key and performs the read.
  BDDNode *data_plane_read_success;
};

rebuilt_bdd_result_t rebuild_bdd(EP *new_ep, const read_write_pattern_t &pattern, const fcfs_cached_table_data_t &fcfs_cached_table_data,
                                 const symbol_t &collision_detected, klee::ref<klee::Expr> collision_detected_condition,
                                 const symbol_t &index_allocation_failed, klee::ref<klee::Expr> index_allocation_failed_condition,
                                 u32 cache_capacity) {
  rebuilt_bdd_result_t result;

  const BDD *old_bdd = new_ep->get_bdd();
  result.bdd         = std::make_unique<BDD>(*old_bdd);

  Call *new_map_get = dynamic_cast<Call *>(result.bdd->get_mutable_node_by_id(pattern.map_get->get_id()));
  new_map_get->add_local_symbol(collision_detected);
  new_map_get->add_local_symbol(index_allocation_failed);

  // delete_coalescing_nodes(new_ep, result.bdd.get(), on_cache_hit, fcfs_cached_table_data.map_objs, fcfs_cached_table_data.original_key,
  //                         deleted_branch_constraints);

  result.bdd->delete_until(new_map_get->get_next()->get_id(), {});

  Branch *collision_detected_branch = result.bdd->create_new_branch(collision_detected_condition);
  new_map_get->set_next(collision_detected_branch);
  collision_detected_branch->set_prev(new_map_get);

  result.on_collision_detected = collision_detected_branch->get_mutable_on_true();

  Branch *index_allocation_failed_branch = result.bdd->create_new_branch(index_allocation_failed_condition);
  collision_detected_branch->set_on_false(index_allocation_failed_branch);
  index_allocation_failed_branch->set_prev(collision_detected_branch);

  result.on_index_allocation_failed = index_allocation_failed_branch->get_mutable_on_true();

  BDDViz::visualize(result.bdd.get(), true);

  // ======================================================================
  // bdd_node_id_t &id              = bdd->get_mutable_id();
  // BDDNodeManager &manager        = bdd->get_mutable_manager();
  // BDDNode *map_get_on_cache_miss = map_get->clone(manager, false);
  // map_get_on_cache_miss->recursive_update_ids(id);
  // new_on_cache_miss = bdd->add_cloned_non_branches(cache_hit_branch->get_on_false()->get_id(), {map_get_on_cache_miss});
  // ======================================================================

  // add_map_get_clone_on_cache_miss(result.bdd.get(), map_get, cache_hit_branch, on_cache_miss);

  // const hit_rate_t cache_success_probability = get_cache_op_success_probability(new_ep, map_get, fcfs_cached_table_data.original_key,
  // cache_capacity); new_ep->get_mutable_ctx().get_mutable_profiler().insert_relative(new_ep->get_active_leaf().node->get_constraints(),
  // cache_hit_condition,
  //                                                                  cache_success_probability);

  // replicate_hdr_parsing_ops_on_cache_miss(new_ep, result.bdd.get(), cache_hit_branch, on_cache_miss);

  // std::vector<klee::ref<klee::Expr>> deleted_branch_constraints;
  // delete_coalescing_nodes_on_success(new_ep, result.bdd.get(), on_cache_hit, fcfs_cached_table_data.map_objs, fcfs_cached_table_data.original_key,
  //                                    deleted_branch_constraints);

  // if (!deleted_branch_constraints.empty()) {
  //   new_ep->get_mutable_ctx().get_mutable_profiler().remove(deleted_branch_constraints);
  // }

  return result;
}

std::unique_ptr<EP> concretize(const EP *ep, const read_write_pattern_t &pattern, const fcfs_cached_table_data_t &fcfs_cached_table_data,
                               const symbol_t &collision_detected, const symbol_t &index_allocation_failed, u32 cache_capacity) {
  FCFSCachedTable *cached_table = TofinoModuleFactory::build_or_reuse_fcfs_cached_table(
      ep, pattern.map_get, fcfs_cached_table_data.obj, fcfs_cached_table_data.original_key, fcfs_cached_table_data.capacity, cache_capacity);

  if (!cached_table) {
    return nullptr;
  }

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  klee::ref<klee::Expr> collision_detected_condition =
      solver_toolbox.exprBuilder->Ne(collision_detected.expr, solver_toolbox.exprBuilder->Constant(0, collision_detected.expr->getWidth()));

  klee::ref<klee::Expr> index_allocation_failed_condition =
      solver_toolbox.exprBuilder->Ne(index_allocation_failed.expr, solver_toolbox.exprBuilder->Constant(0, index_allocation_failed.expr->getWidth()));

  rebuilt_bdd_result_t rebuilt_bdd_result =
      rebuild_bdd(new_ep.get(), pattern, fcfs_cached_table_data, collision_detected, collision_detected_condition, index_allocation_failed,
                  index_allocation_failed_condition, cache_capacity);

  // Symbols symbols = TofinoModuleFactory::get_relevant_dataplane_state(ep, node);

  // Module *module = new FCFSCachedTableReadWrite(node, cached_table->id, cached_table->tables.back().id, fcfs_cached_table_data.obj,
  //                                               fcfs_cached_table_data.keys, fcfs_cached_table_data.read_value, fcfs_cached_table_data.write_value,
  //                                               fcfs_cached_table_data.map_has_this_key, collision_detected, index_allocation_failed);

  // Module *if_module                 = new If(node, cache_hit_condition, {cache_hit_condition});
  // Module *then_module               = new Then(node);
  // Module *else_module               = new Else(node);
  // Module *send_to_controller_module = new SendToController(on_cache_miss, symbols);

  // EPNode *cached_table_cond_write_node = new EPNode(module);
  // EPNode *if_node                 = new EPNode(if_module);
  // EPNode *then_node               = new EPNode(then_module);
  // EPNode *else_node               = new EPNode(else_module);
  // EPNode *send_to_controller_node = new EPNode(send_to_controller_module);

  // TODO: create the EP with the index allocation failed and collision detected checking nodes, and
  // also create a FCFSCachedTableWrite controller module on the control plane for when a collision is detected and we
  // need to send to the controller to update the FCFS Cached Table.

  // cached_table_cond_write_node->set_children(if_node);

  // if_node->set_prev(cached_table_cond_write_node);
  // if_node->set_children(cache_hit_condition, then_node, else_node);

  // then_node->set_prev(if_node);

  // else_node->set_prev(if_node);
  // else_node->set_children(send_to_controller_node);

  // send_to_controller_node->set_prev(else_node);

  // Context &ctx = new_ep->get_mutable_ctx();
  // ctx.save_ds_impl(fcfs_cached_table_data.map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  // ctx.save_ds_impl(fcfs_cached_table_data.map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  // TofinoContext *tofino_ctx = TofinoModuleFactory::get_mutable_tofino_ctx(new_ep.get());
  // tofino_ctx->place(new_ep.get(), node, fcfs_cached_table_data.map_objs.map, cached_table);

  // EPLeaf on_cache_hit_leaf(then_node, on_cache_hit);
  // EPLeaf on_cache_miss_leaf(send_to_controller_node, on_cache_miss);

  // new_ep->process_leaf(cached_table_cond_write_node, {on_cache_hit_leaf, on_cache_miss_leaf});
  // new_ep->replace_bdd(std::move(rebuilt_bdd_result.bdd));

  // const hit_rate_t hr = new_ep->get_ctx().get_profiler().get_hr(send_to_controller_node);
  // new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_traffic(new_ep->get_node_egress(hr, send_to_controller_node));

  return new_ep;
}
} // namespace

std::optional<spec_impl_t> FCFSCachedTableReadWriteFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  read_write_pattern_t pattern;
  if (!is_read_write_pattern(ep->get_bdd(), node, pattern)) {
    return {};
  }

  const std::optional<fcfs_cached_table_data_t> fcfs_cached_table_data =
      build_fcfs_cached_table_data(ep->get_bdd(), ep->get_ctx(), pattern.map_get, pattern.map_put);
  if (!fcfs_cached_table_data.has_value()) {
    return {};
  }

  if (!ctx.can_impl_ds(fcfs_cached_table_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ctx.can_impl_ds(fcfs_cached_table_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  if (const EPNode *ep_node_leaf = ep->get_leaf_ep_node_from_bdd_node(node)) {
    if (was_ds_already_used(ep_node_leaf, build_fcfs_cached_table_id(fcfs_cached_table_data->map_objs.map))) {
      return {};
    }
  }

  const std::vector<u32> allowed_cache_capacities = enum_fcfs_cache_cap(fcfs_cached_table_data->capacity);

  hit_rate_t chosen_success_probability = 0_hr;
  u32 chosen_cache_capacity             = 0;
  bool successfully_placed              = false;

  // We can use a different method for picking the right estimation depending
  // on the time it takes to find a solution.
  for (u32 cache_capacity : allowed_cache_capacities) {
    const hit_rate_t success_probability = get_cache_op_success_probability(ep, node, fcfs_cached_table_data->original_key, cache_capacity);

    if (!can_build_or_reuse_fcfs_cached_table(ep, node, fcfs_cached_table_data->obj, fcfs_cached_table_data->original_key,
                                              fcfs_cached_table_data->capacity, cache_capacity)) {
      continue;
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

  new_ctx.save_ds_impl(fcfs_cached_table_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  new_ctx.save_ds_impl(fcfs_cached_table_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  new_ctx.get_mutable_perf_oracle().add_controller_traffic(on_fail_hr);
  for (const Route *route : node->get_future_routing_decisions()) {
    const fwd_stats_t fwd_stats = new_ctx.get_profiler().get_fwd_stats(route);
    assert(fwd_stats.operation == route->get_operation());
    switch (route->get_operation()) {
    case RouteOp::Forward: {
      for (const auto &[device, dev_hr] : fwd_stats.ports) {
        port_ingress_t node_egress;
        node_egress.controller = hit_rate_t(dev_hr * (1 - chosen_success_probability));
        new_ctx.get_mutable_perf_oracle().add_fwd_traffic(device, node_egress);
      }
    } break;
    case RouteOp::Drop: {
      new_ctx.get_mutable_perf_oracle().add_dropped_traffic(hit_rate_t(fwd_stats.drop * (1 - chosen_success_probability)));
    } break;
    case RouteOp::Broadcast: {
      port_ingress_t node_egress;
      node_egress.controller = hit_rate_t(fwd_stats.flood * (1 - chosen_success_probability));
      for (const auto &[device, _] : fwd_stats.ports) {
        new_ctx.get_mutable_perf_oracle().add_fwd_traffic(device, node_egress);
      }
    } break;
    }
  }

  new_ctx.get_mutable_profiler().scale(node->get_ordered_branch_constraints(), chosen_success_probability.value);

  spec_impl_t spec_impl(decide(ep, node, {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, chosen_cache_capacity}}), new_ctx);

  std::vector<const BDDNode *> ignore_nodes =
      get_nodes_to_speculatively_ignore(ep, pattern.map_get, fcfs_cached_table_data->map_objs, fcfs_cached_table_data->original_key);
  for (const BDDNode *op : ignore_nodes) {
    spec_impl.skip.insert(op->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> FCFSCachedTableReadWriteFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  read_write_pattern_t pattern;
  if (!is_read_write_pattern(ep->get_bdd(), node, pattern)) {
    return {};
  }

  const std::optional<fcfs_cached_table_data_t> fcfs_cached_table_data =
      build_fcfs_cached_table_data(ep->get_bdd(), ep->get_ctx(), pattern.map_get, pattern.map_put);
  if (!fcfs_cached_table_data.has_value()) {
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

  const symbol_t collision_detected               = symbol_manager->create_symbol("collision_detected", 8);
  const symbol_t index_allocation_failed          = symbol_manager->create_symbol("index_allocation_failed", 8);
  const std::vector<u32> allowed_cache_capacities = enum_fcfs_cache_cap(fcfs_cached_table_data->capacity);

  std::vector<impl_t> impls;
  for (u32 cache_capacity : allowed_cache_capacities) {
    std::unique_ptr<EP> new_ep = concretize(ep, pattern, fcfs_cached_table_data.value(), collision_detected, index_allocation_failed, cache_capacity);
    if (new_ep) {
      impl_t impl = implement(ep, node, std::move(new_ep), {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, cache_capacity}});
      impls.push_back(std::move(impl));
    }
  }

  return impls;
}

std::unique_ptr<Module> FCFSCachedTableReadWriteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  // TODO: implement this, but we don't actually use it now.
  return {};
}

} // namespace Tofino
} // namespace LibSynapse