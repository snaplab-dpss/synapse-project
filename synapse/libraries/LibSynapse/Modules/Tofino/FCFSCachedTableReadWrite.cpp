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

struct fcfs_ct_data_t {
  addr_t obj;
  klee::ref<klee::Expr> original_key;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;
  symbol_t map_has_this_key;
  u32 capacity;
  map_coalescing_objs_t map_objs;
};

std::optional<fcfs_ct_data_t> build_fcfs_ct_data(const BDD *bdd, const Context &ctx, const Call *map_get, const Call *map_put) {
  const call_t &get_call = map_get->get_call();
  const call_t &put_call = map_put->get_call();

  fcfs_ct_data_t data;
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
  branch_direction_t index_alloc_success_direction;
  const BDDNode *on_read_success;
  const BDDNode *on_read_failure;
  const BDDNode *on_write_success;
  const BDDNode *on_write_failure;
  symbol_t map_has_this_key;
  symbol_t new_index;
  symbol_t index_allocation_success;
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

  pattern.index_alloc_success_direction = bdd->find_branch_checking_index_alloc(dchain_allocate_new_index);
  if (pattern.index_alloc_success_direction.branch == nullptr) {
    return false;
  }

  const BDDNode *on_index_allocation_success = pattern.index_alloc_success_direction.get_success_node();
  const BDDNode *on_index_allocation_failure = pattern.index_alloc_success_direction.get_failure_node();

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
  pattern.map_has_this_key          = map_get->get_local_symbol("map_has_this_key");
  pattern.new_index                 = dchain_allocate_new_index->get_local_symbol("new_index");
  pattern.index_allocation_success  = dchain_allocate_new_index->get_local_symbol("not_out_of_space");

  return true;
}

// BDDNode *replicate_hdr_parsing_ops_on_collision_detected(const EP *ep, BDD *bdd, const Branch *collision_detected) {
//   const BDDNode *on_collision_detected = collision_detected->get_on_true();

//   std::list<const Call *> prev_borrows =
//       on_collision_detected->get_prev_functions({"packet_borrow_next_chunk"}, ep->get_target_roots(ep->get_active_target()));

//   if (prev_borrows.empty()) {
//     return nullptr;
//   }

//   std::vector<const BDDNode *> non_branch_nodes_to_add;
//   for (const Call *prev_borrow : prev_borrows) {
//     non_branch_nodes_to_add.push_back(prev_borrow);
//   }

//   return bdd->add_cloned_non_branches(on_collision_detected->get_id(), non_branch_nodes_to_add);
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
  }

  return nodes_to_ignore;
}

struct rebuilt_bdd_result_t {
  std::unique_ptr<BDD> bdd;

  // For when we require the control plane to perform the write operation.
  BDDNode *on_collision_detected;

  // For when the data plane fails to find the key but successfully allocates a new index and performs the write.
  BDDNode *data_plane_write_success;

  // For when the data plane successfully finds the key and performs the read.
  BDDNode *data_plane_read_success;
};

rebuilt_bdd_result_t rebuild_bdd(EP *new_ep, const read_write_pattern_t &pattern, const fcfs_ct_data_t &fcfs_ct_data,
                                 const symbol_t &collision_detected, klee::ref<klee::Expr> collision_detected_condition, u32 cache_capacity) {
  rebuilt_bdd_result_t result;

  // const BDD *old_bdd = new_ep->get_bdd();
  // result.bdd         = std::make_unique<BDD>(*old_bdd);

  // Call *new_map_get = dynamic_cast<Call *>(result.bdd->get_mutable_node_by_id(pattern.map_get->get_id()));
  // new_map_get->add_local_symbol(collision_detected);

  // BDDNode *new_dchain_allocate_new_index = pattern.dchain_allocate_new_index->clone(result.bdd->get_mutable_manager(), true);
  // new_dchain_allocate_new_index->recursive_update_ids(result.bdd->get_mutable_id());

  // BDDNode *on_write_success = result.bdd->get_mutable_node_by_id(pattern.on_write_success->get_id());
  // Branch *collision_detected_branch =
  //     result.bdd->add_cloned_branch(on_write_success->get_id(), collision_detected_condition, new_dchain_allocate_new_index, on_write_success);

  // result.data_plane_read_success  = result.bdd->get_mutable_node_by_id(pattern.on_read_success->get_id());
  // result.data_plane_write_success = collision_detected_branch->get_mutable_on_false();

  // BDDNode *new_on_collision_detected_with_hdr_parsing =
  //     replicate_hdr_parsing_ops_on_collision_detected(new_ep, result.bdd.get(), collision_detected_branch);
  // if (new_on_collision_detected_with_hdr_parsing == nullptr) {
  //   result.on_collision_detected = collision_detected_branch->get_mutable_on_true();
  // } else {
  //   result.on_collision_detected = new_on_collision_detected_with_hdr_parsing;
  // }

  // const std::vector<klee::ref<klee::Expr>> index_alloc_failed_on_dataplane_constraints =
  //     pattern.index_alloc_success_direction.get_failure_node()->get_ordered_branch_constraints();

  // result.bdd->delete_branch(pattern.index_alloc_success_direction.branch->get_id(), pattern.index_alloc_success_direction.direction
  //                                                                                       ? BDD::BranchDeletionAction::KeepOnTrue
  //                                                                                       : BDD::BranchDeletionAction::KeepOnFalse);

  // const hit_rate_t cache_collision_probability =
  //     TofinoModuleFactory::get_fcfs_ct_cache_collision_probability(new_ep->get_ctx(), pattern.map_put, fcfs_ct_data.original_key, cache_capacity);

  // new_ep->get_mutable_ctx().get_mutable_profiler().insert_relative(pattern.on_write_success->get_ordered_branch_constraints(),
  //                                                                  collision_detected_condition, cache_collision_probability);
  // new_ep->get_mutable_ctx().get_mutable_profiler().remove(index_alloc_failed_on_dataplane_constraints);

  // BDDViz::visualize(result.bdd.get(), false);
  // const BDD::inspection_report_t bdd_inspection_report = result.bdd->inspect();
  // if (bdd_inspection_report.status != BDD::InspectionStatus::Ok) {
  //   panic("BDD inspection failed: %s", bdd_inspection_report.message.c_str());
  // }
  // ProfilerViz::visualize(result.bdd.get(), new_ep->get_ctx().get_profiler(), false);
  // dbg_pause();

  return result;
}

std::unique_ptr<EP> concretize(const EP *ep, const read_write_pattern_t &pattern, const fcfs_ct_data_t &fcfs_ct_data,
                               const symbol_t &collision_detected, u32 cache_capacity) {
  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  FCFSCachedTable *cached_table = TofinoModuleFactory::build_or_reuse_fcfs_ct(new_ep.get(), pattern.map_get, fcfs_ct_data.obj,
                                                                              fcfs_ct_data.original_key, fcfs_ct_data.capacity, cache_capacity);
  if (!cached_table) {
    return nullptr;
  }

  klee::ref<klee::Expr> map_has_this_key_condition = solver_toolbox.exprBuilder->Ne(
      pattern.map_has_this_key.expr, solver_toolbox.exprBuilder->Constant(0, pattern.map_has_this_key.expr->getWidth()));
  klee::ref<klee::Expr> index_allocation_success_condition = solver_toolbox.exprBuilder->Ne(
      pattern.index_allocation_success.expr, solver_toolbox.exprBuilder->Constant(0, pattern.index_allocation_success.expr->getWidth()));
  klee::ref<klee::Expr> collision_detected_condition =
      solver_toolbox.exprBuilder->Ne(collision_detected.expr, solver_toolbox.exprBuilder->Constant(0, collision_detected.expr->getWidth()));

  rebuilt_bdd_result_t rebuilt_bdd_result =
      rebuild_bdd(new_ep.get(), pattern, fcfs_ct_data, collision_detected, collision_detected_condition, cache_capacity);

  const Symbols symbols = TofinoModuleFactory::get_relevant_dataplane_state(ep, pattern.on_write_success);

  Module *module = new FCFSCachedTableReadWrite(pattern.map_get, cached_table->id, cached_table->tables.back().id, fcfs_ct_data.obj,
                                                fcfs_ct_data.keys, fcfs_ct_data.read_value, fcfs_ct_data.write_value, fcfs_ct_data.map_has_this_key,
                                                collision_detected, pattern.index_allocation_success);

  Module *if_read_module   = new If(pattern.map_get, map_has_this_key_condition, {map_has_this_key_condition});
  Module *read_then_module = new Then(pattern.map_get);
  Module *read_else_module = new Else(pattern.map_get);

  Module *if_collision_detected_module   = new If(pattern.map_get, collision_detected_condition, {collision_detected_condition});
  Module *collision_detected_then_module = new Then(pattern.map_get);
  Module *collision_detected_else_module = new Else(pattern.map_get);

  Module *send_to_controller_module = new SendToController(pattern.map_get, symbols);

  EPNode *fcfs_ct_read_write_node = new EPNode(module);

  EPNode *if_read_node   = new EPNode(if_read_module);
  EPNode *read_then_node = new EPNode(read_then_module);
  EPNode *read_else_node = new EPNode(read_else_module);

  EPNode *if_collision_detected_node   = new EPNode(if_collision_detected_module);
  EPNode *collision_detected_then_node = new EPNode(collision_detected_then_module);
  EPNode *collision_detected_else_node = new EPNode(collision_detected_else_module);

  EPNode *send_to_controller_node = new EPNode(send_to_controller_module);

  fcfs_ct_read_write_node->set_children(if_read_node);
  if_read_node->set_prev(fcfs_ct_read_write_node);

  if_read_node->set_children(map_has_this_key_condition, read_then_node, read_else_node);
  read_then_node->set_prev(if_read_node);
  read_else_node->set_prev(if_read_node);

  read_else_node->set_children(if_collision_detected_node);
  if_collision_detected_node->set_prev(read_else_node);

  if_collision_detected_node->set_children(collision_detected_condition, collision_detected_then_node, collision_detected_else_node);
  collision_detected_then_node->set_prev(if_collision_detected_node);
  collision_detected_else_node->set_prev(if_collision_detected_node);

  collision_detected_then_node->set_children(send_to_controller_node);
  send_to_controller_node->set_prev(collision_detected_then_node);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(fcfs_ct_data.map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  ctx.save_ds_impl(fcfs_ct_data.map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  TofinoContext *tofino_ctx = TofinoModuleFactory::get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), pattern.map_get, fcfs_ct_data.map_objs.map, cached_table);

  EPLeaf on_read_leaf(read_then_node, rebuilt_bdd_result.data_plane_read_success);
  EPLeaf on_collision_detected_leaf(send_to_controller_node, rebuilt_bdd_result.on_collision_detected);
  EPLeaf on_write_success_leaf(collision_detected_else_node, rebuilt_bdd_result.data_plane_write_success);

  new_ep->process_leaf(fcfs_ct_read_write_node, {on_read_leaf, on_collision_detected_leaf, on_write_success_leaf});
  new_ep->replace_bdd(std::move(rebuilt_bdd_result.bdd));

  const hit_rate_t hr = new_ep->get_ctx().get_profiler().get_hr(send_to_controller_node);
  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_traffic(new_ep->get_node_egress(hr, send_to_controller_node));

  return new_ep;
}
} // namespace

std::optional<spec_impl_t> FCFSCachedTableReadWriteFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  read_write_pattern_t pattern;
  if (!is_read_write_pattern(ep->get_bdd(), node, pattern)) {
    return {};
  }

  const std::optional<fcfs_ct_data_t> fcfs_ct_data = build_fcfs_ct_data(ep->get_bdd(), ep->get_ctx(), pattern.map_get, pattern.map_put);
  if (!fcfs_ct_data.has_value()) {
    return {};
  }

  if (!ctx.can_impl_ds(fcfs_ct_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ctx.can_impl_ds(fcfs_ct_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  if (ep->get_id() == 69) {
    if (const EPNode *ep_node_leaf = ep->get_leaf_ep_node_from_bdd_node(node)) {
      std::cerr << "EP node leaf: " << ep_node_leaf->dump() << "\n";
      if (was_ds_already_used(ep_node_leaf, build_fcfs_ct_id(fcfs_ct_data->map_objs.map))) {
        std::cerr << "FCFSCachedTableReadWriteFactory::speculate: skipping speculation on EP 69 for DS already used\n";
      }
    }
  }

  if (const EPNode *ep_node_leaf = ep->get_leaf_ep_node_from_bdd_node(node)) {
    if (was_ds_already_used(ep_node_leaf, build_fcfs_ct_id(fcfs_ct_data->map_objs.map))) {
      return {};
    }
  }

  const std::vector<u32> allowed_cache_capacities = enum_fcfs_ct_cache_capacities();

  hit_rate_t chosen_collision_probability = 1_hr;
  u32 chosen_cache_capacity               = 0;
  bool successfully_placed                = false;

  // We can use a different method for picking the right estimation depending on the time it takes to find a solution.
  for (u32 cache_capacity : allowed_cache_capacities) {
    const hit_rate_t cache_collision_probability =
        get_fcfs_ct_cache_collision_probability(ep->get_ctx(), pattern.map_put, fcfs_ct_data->original_key, cache_capacity);

    if (!can_build_or_reuse_fcfs_ct(ep, node, fcfs_ct_data->obj, fcfs_ct_data->original_key, fcfs_ct_data->capacity, cache_capacity)) {
      continue;
    }

    if (cache_collision_probability < chosen_collision_probability) {
      chosen_collision_probability = cache_collision_probability;
      chosen_cache_capacity        = cache_capacity;
    }

    successfully_placed = true;
  }

  if (!successfully_placed) {
    return {};
  }

  Context new_ctx = ctx;

  const hit_rate_t hr         = new_ctx.get_profiler().get_hr(pattern.on_write_success);
  const hit_rate_t on_fail_hr = hit_rate_t{hr * chosen_collision_probability};

  new_ctx.save_ds_impl(fcfs_ct_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  new_ctx.save_ds_impl(fcfs_ct_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  new_ctx.get_mutable_perf_oracle().add_controller_traffic(on_fail_hr);
  for (const Route *route : pattern.on_write_success->get_future_routing_decisions()) {
    const fwd_stats_t fwd_stats = new_ctx.get_profiler().get_fwd_stats(route);
    assert(fwd_stats.operation == route->get_operation());
    switch (route->get_operation()) {
    case RouteOp::Forward: {
      for (const auto &[device, dev_hr] : fwd_stats.ports) {
        port_ingress_t node_egress;
        node_egress.controller = hit_rate_t(dev_hr * chosen_collision_probability);
        new_ctx.get_mutable_perf_oracle().add_fwd_traffic(device, node_egress);
      }
    } break;
    case RouteOp::Drop: {
      new_ctx.get_mutable_perf_oracle().add_dropped_traffic(hit_rate_t(fwd_stats.drop * chosen_collision_probability));
    } break;
    case RouteOp::Broadcast: {
      port_ingress_t node_egress;
      node_egress.controller = hit_rate_t(fwd_stats.flood * chosen_collision_probability);
      for (const auto &[device, _] : fwd_stats.ports) {
        new_ctx.get_mutable_perf_oracle().add_fwd_traffic(device, node_egress);
      }
    } break;
    }
  }

  new_ctx.get_mutable_profiler().scale(pattern.on_write_success->get_ordered_branch_constraints(), 1 - chosen_collision_probability.value);

  spec_impl_t spec_impl(decide(ep, node, {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, chosen_cache_capacity}}), new_ctx);

  std::vector<const BDDNode *> ignore_nodes =
      get_nodes_to_speculatively_ignore(ep, pattern.map_get, fcfs_ct_data->map_objs, fcfs_ct_data->original_key);
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

  const std::optional<fcfs_ct_data_t> fcfs_ct_data = build_fcfs_ct_data(ep->get_bdd(), ep->get_ctx(), pattern.map_get, pattern.map_put);
  if (!fcfs_ct_data.has_value()) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(fcfs_ct_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ep->get_ctx().can_impl_ds(fcfs_ct_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  if (const EPNode *ep_node_leaf = ep->get_leaf_ep_node_from_bdd_node(node)) {
    if (was_ds_already_used(ep_node_leaf, build_fcfs_ct_id(fcfs_ct_data->map_objs.map))) {
      return {};
    }
  }

  const symbol_t collision_detected               = symbol_manager->create_symbol("collision_detected", 32);
  const std::vector<u32> allowed_cache_capacities = enum_fcfs_ct_cache_capacities();

  std::vector<impl_t> impls;
  for (u32 cache_capacity : allowed_cache_capacities) {
    std::unique_ptr<EP> new_ep = concretize(ep, pattern, fcfs_ct_data.value(), collision_detected, cache_capacity);
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