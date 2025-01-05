#include "hh_table_conditional_update.h"

#include "if.h"
#include "then.h"
#include "else.h"
#include "send_to_controller.h"
#include "hh_table_read.h"
#include "../controller/hh_table_update.h"

namespace synapse {
namespace tofino {
namespace {
struct hh_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  std::vector<klee::ref<klee::Expr>> table_keys;
  klee::ref<klee::Expr> value;

  hh_table_data_t(const Call *map_put) {
    const call_t &call = map_put->get_call();
    SYNAPSE_ASSERT(call.function_name == "map_put", "Not a map_put call");

    obj = expr_addr_to_obj_addr(call.args.at("map").expr);
    key = call.args.at("key").in;
    table_keys = Table::build_keys(key);
    value = call.args.at("value").expr;
  }
};

klee::ref<klee::Expr> get_min_estimate(const EP *ep) {
  EPLeaf leaf = ep->get_active_leaf();
  const EPNode *node = leaf.node;

  while (node) {
    if (node->get_module()->get_type() == ModuleType::Tofino_HHTableRead) {
      const HHTableRead *hh_table_read =
          dynamic_cast<const HHTableRead *>(node->get_module());
      return hh_table_read->get_min_estimate();
    }
    node = node->get_prev();
  }

  return nullptr;
}

klee::ref<klee::Expr> build_min_estimate_check_cond(const EP *ep,
                                                    klee::ref<klee::Expr> min_estimate,
                                                    addr_t map) {
  const std::unordered_set<DS *> &data_structures =
      ep->get_ctx().get_target_ctx<TofinoContext>()->get_ds(map);
  SYNAPSE_ASSERT(data_structures.size() == 1, "Multiple data structures found");
  const DS *ds = *data_structures.begin();
  SYNAPSE_ASSERT(ds->type == DSType::HH_TABLE, "Not a heavy hitter table");
  const HHTable *hh_table = dynamic_cast<const HHTable *>(ds);

  u32 topk = hh_table->num_entries;
  u64 threshold =
      ep->get_ctx().get_profiler().get_bdd_profile()->threshold_top_k_flows(map, topk);
  klee::ref<klee::Expr> threshold_expr =
      solver_toolbox.exprBuilder->Constant(threshold, 32);
  klee::ref<klee::Expr> condition =
      solver_toolbox.exprBuilder->Ugt(min_estimate, threshold_expr);
  return condition;
}

// hit_rate_t get_hh_probability(const EP *ep, const Node *node, addr_t map) {
//   hit_rate_t node_hr = ep->get_ctx().get_profiler().get_hr(node);
//   u32 capacity = ep->get_ctx().get_map_config(map).capacity;
//   hit_rate_t churn_hr = ep->get_ctx()
//                             .get_profiler()
//                             .get_bdd_profile()
//                             ->churn_hit_rate_top_k_flows(map, capacity);
//   return node_hr * churn_hr;
// }

const Call *get_future_map_put(const Node *node, addr_t map) {
  for (const Call *map_put : get_future_functions(node, {"map_put"})) {
    const call_t &call = map_put->get_call();
    klee::ref<klee::Expr> map_expr = call.args.at("map").expr;

    if (expr_addr_to_obj_addr(map_expr) == map) {
      return map_put;
    }
  }

  return nullptr;
}

std::unique_ptr<BDD> rebuild_bdd(const EP *ep, const Node *dchain_allocate_new_index,
                                 const map_coalescing_objs_t &map_objs,
                                 klee::ref<klee::Expr> key,
                                 klee::ref<klee::Expr> min_estimate_cond,
                                 Branch *&min_estimate_cond_branch) {
  const BDD *old_bdd = ep->get_bdd();
  std::unique_ptr<BDD> bdd = std::make_unique<BDD>(*old_bdd);

  Node *node =
      delete_non_branch_node_from_bdd(bdd.get(), dchain_allocate_new_index->get_id());

  min_estimate_cond_branch = add_branch_to_bdd(bdd.get(), node, min_estimate_cond);

  // FIXME: assuming index allocation is successful on true.
  Node *on_hh = delete_branch_node_from_bdd(
      bdd.get(), min_estimate_cond_branch->get_mutable_on_true()->get_id(), true);
  Node *on_not_hh = delete_branch_node_from_bdd(
      bdd.get(), min_estimate_cond_branch->get_mutable_on_false()->get_id(), false);

  // Add the header parsing operations to the HH branch side, which goes to
  // the controller.
  std::vector<const Call *> prev_borrows =
      get_prev_functions(ep, on_hh, {"packet_borrow_next_chunk"});
  std::vector<const Call *> prev_returns =
      get_prev_functions(ep, on_hh, {"packet_return_chunk"});

  std::vector<const Node *> hdr_parsing_ops;
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_borrows.begin(), prev_borrows.end());
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_returns.begin(), prev_returns.end());

  on_hh = add_non_branch_nodes_to_bdd(bdd.get(), on_hh, hdr_parsing_ops);

  // Remove the coalescing nodes from the not HH branch side.
  std::vector<const Call *> targets =
      get_coalescing_nodes_from_key(bdd.get(), on_hh, key, map_objs);

  while (on_hh && !targets.empty()) {
    auto found_it =
        std::find_if(targets.begin(), targets.end(), [&on_hh](const Call *target) {
          return target->get_id() == on_hh->get_id();
        });

    if (found_it != targets.end()) {
      on_hh = delete_non_branch_node_from_bdd(bdd.get(), on_hh->get_id());
      targets.erase(found_it);
    } else {
      SYNAPSE_ASSERT(on_hh->get_type() != NodeType::Branch, "Unexpected branch");
      on_hh = on_hh->get_mutable_next();
    }
  }

  SYNAPSE_ASSERT(targets.empty(), "Not all coalescing nodes removed");

  return bdd;
}
} // namespace

using ctrl::HHTableUpdate;

std::optional<spec_impl_t>
HHTableConditionalUpdateFactory::speculate(const EP *ep, const Node *node,
                                           const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!is_map_update_with_dchain(ep, dchain_allocate_new_index, future_map_puts)) {
    return std::nullopt;
  }

  if (!is_index_alloc_on_unsuccessful_map_get(ep, dchain_allocate_new_index)) {
    return std::nullopt;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_dchain_op(ep, dchain_allocate_new_index, map_objs)) {
    return std::nullopt;
  }

  const Call *map_put = get_future_map_put(node, map_objs.map);
  SYNAPSE_ASSERT(map_put, "map_put not found");

  hh_table_data_t table_data(map_put);

  if (!ctx.check_ds_impl(map_objs.map, DSImpl::Tofino_HeavyHitterTable) ||
      !ctx.check_ds_impl(map_objs.dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return std::nullopt;
  }

  hit_rate_t node_hr = ep->get_ctx().get_profiler().get_hr(node);
  u32 capacity = ep->get_ctx().get_map_config(map_objs.map).capacity;
  hit_rate_t new_hh_probability =
      ep->get_ctx().get_profiler().get_bdd_profile()->churn_hit_rate_top_k_flows(
          map_objs.map, capacity);
  hit_rate_t new_hh_hr = node_hr * new_hh_probability;

  Context new_ctx = ctx;
  new_ctx.get_mutable_perf_oracle().add_controller_traffic(new_hh_hr);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);

  // Get all nodes executed on a successful index allocation.
  branch_direction_t index_alloc_check =
      find_branch_checking_index_alloc(ep, dchain_allocate_new_index);
  SYNAPSE_ASSERT(index_alloc_check.direction,
                 "Branch checking index allocation not found");

  const Node *on_hh = index_alloc_check.direction
                          ? index_alloc_check.branch->get_on_true()
                          : index_alloc_check.branch->get_on_false();

  std::vector<const Call *> targets =
      get_coalescing_nodes_from_key(ep->get_bdd(), on_hh, table_data.key, map_objs);

  // Ignore all coalescing nodes if the index allocation is successful (i.e.
  // it is a heavy hitter), as we are sending everything to the controller.
  for (const Node *target : targets) {
    spec_impl.skip.insert(target->get_id());
  }

  return spec_impl;
}

std::vector<impl_t>
HHTableConditionalUpdateFactory::process_node(const EP *ep, const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!is_map_update_with_dchain(ep, dchain_allocate_new_index, future_map_puts)) {
    return impls;
  }

  if (!is_index_alloc_on_unsuccessful_map_get(ep, dchain_allocate_new_index)) {
    return impls;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_dchain_op(ep, dchain_allocate_new_index, map_objs)) {
    return impls;
  }

  branch_direction_t index_alloc_check =
      find_branch_checking_index_alloc(ep, dchain_allocate_new_index);
  if (dchain_allocate_new_index->get_next() != index_alloc_check.branch) {
    return impls;
  }

  // We require the HH table to be implemented by the HH Table Read module.
  // We actually don't really need this, we could query the CMS right here,
  // but we leave it like this for now.

  if (!ep->get_ctx().check_ds_impl(map_objs.map, DSImpl::Tofino_HeavyHitterTable) ||
      !ep->get_ctx().check_ds_impl(map_objs.dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return impls;
  }

  klee::ref<klee::Expr> min_estimate = get_min_estimate(ep);
  SYNAPSE_ASSERT(!min_estimate.isNull(), "TODO: HHTableRead not found, so we should "
                                         "query the CMS for the min estimate");

  const Call *map_put = get_future_map_put(node, map_objs.map);
  SYNAPSE_ASSERT(map_put, "map_put not found");

  hh_table_data_t table_data(map_put);

  const Node *next_on_hh = index_alloc_check.direction
                               ? index_alloc_check.branch->get_on_true()
                               : index_alloc_check.branch->get_on_false();
  const Node *next_on_not_hh = index_alloc_check.direction
                                   ? index_alloc_check.branch->get_on_false()
                                   : index_alloc_check.branch->get_on_true();

  klee::ref<klee::Expr> min_estimate_cond =
      build_min_estimate_check_cond(ep, min_estimate, map_objs.map);

  symbols_t symbols = get_dataplane_state(ep, node);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Branch *min_estimate_cond_branch;
  std::unique_ptr<BDD> new_bdd =
      rebuild_bdd(new_ep, dchain_allocate_new_index, map_objs, table_data.key,
                  min_estimate_cond, min_estimate_cond_branch);

  Module *if_module =
      new If(min_estimate_cond_branch, min_estimate_cond, {min_estimate_cond});
  Module *then_module = new Then(min_estimate_cond_branch);
  Module *else_module = new Else(min_estimate_cond_branch);
  Module *send_to_controller_module =
      new SendToController(min_estimate_cond_branch->get_on_true(), symbols);
  Module *hh_table_update_module =
      new HHTableUpdate(min_estimate_cond_branch->get_on_true(), table_data.obj,
                        table_data.table_keys, table_data.value, min_estimate);

  EPNode *if_node = new EPNode(if_module);
  EPNode *then_node = new EPNode(then_module);
  EPNode *else_node = new EPNode(else_module);
  EPNode *send_to_controller_node = new EPNode(send_to_controller_module);
  EPNode *hh_table_update_node = new EPNode(hh_table_update_module);

  if_node->set_constraint(min_estimate_cond);
  if_node->set_children(then_node, else_node);

  then_node->set_prev(if_node);
  then_node->set_children(send_to_controller_node);

  else_node->set_prev(if_node);

  send_to_controller_node->set_prev(then_node);
  send_to_controller_node->set_children(hh_table_update_node);

  hh_table_update_node->set_prev(send_to_controller_node);

  hit_rate_t node_hr = ep->get_ctx().get_profiler().get_hr(node);
  u32 capacity = ep->get_ctx().get_map_config(map_objs.map).capacity;
  hit_rate_t new_hh_probability =
      ep->get_ctx().get_profiler().get_bdd_profile()->churn_hit_rate_top_k_flows(
          map_objs.map, capacity);

  new_ep->get_mutable_ctx().get_mutable_profiler().insert_relative(
      new_ep->get_active_leaf().node->get_constraints(), min_estimate_cond,
      new_hh_probability);

  constraints_t lhs_cnstrs = node->get_ordered_branch_constraints();
  lhs_cnstrs.push_back(min_estimate_cond);
  lhs_cnstrs.push_back(index_alloc_check.branch->get_condition());

  constraints_t rhs_cnstrs = node->get_ordered_branch_constraints();
  rhs_cnstrs.push_back(solver_toolbox.exprBuilder->Not(min_estimate_cond));
  rhs_cnstrs.push_back(
      solver_toolbox.exprBuilder->Not(index_alloc_check.branch->get_condition()));

  new_ep->get_mutable_ctx().get_mutable_profiler().remove(lhs_cnstrs);
  new_ep->get_mutable_ctx().get_mutable_profiler().remove(rhs_cnstrs);

  EPLeaf on_hh_leaf(hh_table_update_node, min_estimate_cond_branch->get_on_true());
  EPLeaf on_not_hh_leaf(else_node, min_estimate_cond_branch->get_on_false());

  new_ep->process_leaf(if_node, {on_hh_leaf, on_not_hh_leaf});
  new_ep->replace_bdd(std::move(new_bdd));
  new_ep->assert_integrity();

  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_traffic(
      get_node_egress(new_ep, send_to_controller_node));

  return impls;
}

} // namespace tofino
} // namespace synapse