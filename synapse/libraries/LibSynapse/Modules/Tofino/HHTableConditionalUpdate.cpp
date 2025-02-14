#include <LibSynapse/Modules/Tofino/HHTableConditionalUpdate.h>
#include <LibSynapse/Modules/Tofino/If.h>
#include <LibSynapse/Modules/Tofino/Then.h>
#include <LibSynapse/Modules/Tofino/Else.h>
#include <LibSynapse/Modules/Tofino/SendToController.h>
#include <LibSynapse/Modules/Tofino/HHTableRead.h>
#include <LibSynapse/Modules/Controller/HHTableUpdate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

namespace {
struct hh_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  std::vector<klee::ref<klee::Expr>> table_keys;
  klee::ref<klee::Expr> value;

  hh_table_data_t(const LibBDD::Call *map_put) {
    const LibBDD::call_t &call = map_put->get_call();
    assert(call.function_name == "map_put" && "Not a map_put call");

    obj        = LibCore::expr_addr_to_obj_addr(call.args.at("map").expr);
    key        = call.args.at("key").in;
    table_keys = Table::build_keys(key);
    value      = call.args.at("value").expr;
  }
};

LibCore::symbol_t get_min_estimate(const EP *ep) {
  EPLeaf leaf        = ep->get_active_leaf();
  const EPNode *node = leaf.node;

  while (node) {
    if (node->get_module()->get_type() == ModuleType::Tofino_HHTableRead) {
      const HHTableRead *hh_table_read = dynamic_cast<const HHTableRead *>(node->get_module());
      return hh_table_read->get_min_estimate();
    }
    node = node->get_prev();
  }

  panic("TODO: HHTableRead not found, so we should "
        "query the CMS for the min estimate");
}

klee::ref<klee::Expr> build_min_estimate_check_cond(const EP *ep, const LibCore::symbol_t &min_estimate, addr_t map) {
  const std::unordered_set<DS *> &data_structures = ep->get_ctx().get_target_ctx<TofinoContext>()->get_ds(map);
  assert(data_structures.size() == 1 && "Multiple data structures found");
  const DS *ds = *data_structures.begin();
  assert(ds->type == DSType::HH_TABLE && "Not a heavy hitter table");
  const HHTable *hh_table = dynamic_cast<const HHTable *>(ds);

  u32 topk                             = hh_table->num_entries;
  u64 threshold                        = ep->get_ctx().get_profiler().get_bdd_profile()->threshold_top_k_flows(map, topk);
  klee::ref<klee::Expr> threshold_expr = LibCore::solver_toolbox.exprBuilder->Constant(threshold, 32);
  klee::ref<klee::Expr> condition      = LibCore::solver_toolbox.exprBuilder->Ugt(min_estimate.expr, threshold_expr);
  return condition;
}

// hit_rate_t get_hh_probability(const EP *ep, const LibBDD::Node* node, addr_t map) {
//   hit_rate_t node_hr = ep->get_ctx().get_profiler().get_hr(node);
//   u32 capacity = ep->get_ctx().get_map_config(map).capacity;
//   hit_rate_t churn_hr = ep->get_ctx()
//                             .get_profiler()
//                             .get_bdd_profile()
//                             ->churn_hit_rate_top_k_flows(map, capacity);
//   return node_hr * churn_hr;
// }

const LibBDD::Call *get_future_map_put(const LibBDD::Node *node, addr_t map) {
  for (const LibBDD::Call *map_put : node->get_future_functions({"map_put"})) {
    const LibBDD::call_t &call     = map_put->get_call();
    klee::ref<klee::Expr> map_expr = call.args.at("map").expr;

    if (LibCore::expr_addr_to_obj_addr(map_expr) == map) {
      return map_put;
    }
  }

  return nullptr;
}

std::unique_ptr<LibBDD::BDD> rebuild_bdd(const EP *ep, const LibBDD::Node *dchain_allocate_new_index,
                                         const LibBDD::map_coalescing_objs_t &map_objs, klee::ref<klee::Expr> key,
                                         klee::ref<klee::Expr> min_estimate_cond, LibBDD::Branch *&min_estimate_cond_branch) {
  const LibBDD::BDD *old_bdd       = ep->get_bdd();
  std::unique_ptr<LibBDD::BDD> bdd = std::make_unique<LibBDD::BDD>(*old_bdd);

  LibBDD::Node *node = bdd->delete_non_branch(dchain_allocate_new_index->get_id());

  min_estimate_cond_branch = bdd->clone_and_add_branch(node, min_estimate_cond);

  // FIXME: assuming index allocation is successful on true.
  LibBDD::Node *on_hh = bdd->delete_branch(min_estimate_cond_branch->get_mutable_on_true()->get_id(), true);
  panic("TODO: hh_table_conditional_update::rebuild_bdd");
  // LibBDD::Node *on_not_hh = bdd->delete_branch(
  //     min_estimate_cond_branch->get_mutable_on_false()->get_id(), false);

  // Add the header parsing operations to the HH branch side, which goes to
  // the controller.
  std::vector<const LibBDD::Call *> prev_borrows =
      on_hh->get_prev_functions({"packet_borrow_next_chunk"}, ep->get_target_roots(ep->get_active_target()));
  std::vector<const LibBDD::Call *> prev_returns =
      on_hh->get_prev_functions({"packet_return_chunk"}, ep->get_target_roots(ep->get_active_target()));

  std::vector<const LibBDD::Node *> hdr_parsing_ops;
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_borrows.begin(), prev_borrows.end());
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_returns.begin(), prev_returns.end());

  on_hh = bdd->clone_and_add_non_branches(on_hh, hdr_parsing_ops);

  // Remove the coalescing nodes from the not HH branch side.
  std::vector<const LibBDD::Call *> targets = on_hh->get_coalescing_nodes_from_key(key, map_objs);

  while (on_hh && !targets.empty()) {
    auto found_it =
        std::find_if(targets.begin(), targets.end(), [&on_hh](const LibBDD::Call *target) { return target->get_id() == on_hh->get_id(); });

    if (found_it != targets.end()) {
      on_hh = bdd->delete_non_branch(on_hh->get_id());
      targets.erase(found_it);
    } else {
      assert(on_hh->get_type() != LibBDD::NodeType::Branch && "Unexpected branch");
      on_hh = on_hh->get_mutable_next();
    }
  }

  assert(targets.empty() && "Not all coalescing nodes removed");

  return bdd;
}
} // namespace

using Controller::HHTableUpdate;

std::optional<spec_impl_t> HHTableConditionalUpdateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *dchain_allocate_new_index = dynamic_cast<const LibBDD::Call *>(node);

  std::vector<const LibBDD::Call *> future_map_puts;
  if (!ep->get_bdd()->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts)) {
    return std::nullopt;
  }

  if (!ep->get_bdd()->is_index_alloc_on_unsuccessful_map_get(dchain_allocate_new_index)) {
    return std::nullopt;
  }

  LibBDD::map_coalescing_objs_t map_objs;
  if (!ep->get_bdd()->get_map_coalescing_objs_from_dchain_op(dchain_allocate_new_index, map_objs)) {
    return std::nullopt;
  }

  const LibBDD::Call *map_put = get_future_map_put(node, map_objs.map);
  assert(map_put && "map_put not found");

  hh_table_data_t table_data(map_put);

  if (!ctx.check_ds_impl(map_objs.map, DSImpl::Tofino_HeavyHitterTable) ||
      !ctx.check_ds_impl(map_objs.dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return std::nullopt;
  }

  hit_rate_t node_hr            = ep->get_ctx().get_profiler().get_hr(node);
  u32 capacity                  = ep->get_ctx().get_map_config(map_objs.map).capacity;
  hit_rate_t new_hh_probability = ep->get_ctx().get_profiler().get_bdd_profile()->churn_hit_rate_top_k_flows(map_objs.map, capacity);
  hit_rate_t new_hh_hr          = node_hr * new_hh_probability;

  Context new_ctx = ctx;
  new_ctx.get_mutable_perf_oracle().add_controller_traffic(new_hh_hr);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);

  // Get all nodes executed on a successful index allocation.
  LibBDD::branch_direction_t index_alloc_check = dchain_allocate_new_index->find_branch_checking_index_alloc();
  assert(index_alloc_check.direction && "Branch checking index allocation not found");

  const LibBDD::Node *on_hh =
      index_alloc_check.direction ? index_alloc_check.branch->get_on_true() : index_alloc_check.branch->get_on_false();

  std::vector<const LibBDD::Call *> targets = on_hh->get_coalescing_nodes_from_key(table_data.key, map_objs);

  // Ignore all coalescing nodes if the index allocation is successful (i.e.
  // it is a heavy hitter), as we are sending everything to the controller.
  for (const LibBDD::Node *target : targets) {
    spec_impl.skip.insert(target->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> HHTableConditionalUpdateFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                  LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *dchain_allocate_new_index = dynamic_cast<const LibBDD::Call *>(node);

  std::vector<const LibBDD::Call *> future_map_puts;
  if (!ep->get_bdd()->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts)) {
    return impls;
  }

  if (!ep->get_bdd()->is_index_alloc_on_unsuccessful_map_get(dchain_allocate_new_index)) {
    return impls;
  }

  LibBDD::map_coalescing_objs_t map_objs;
  if (!ep->get_bdd()->get_map_coalescing_objs_from_dchain_op(dchain_allocate_new_index, map_objs)) {
    return impls;
  }

  LibBDD::branch_direction_t index_alloc_check = dchain_allocate_new_index->find_branch_checking_index_alloc();
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

  LibCore::symbol_t min_estimate = get_min_estimate(ep);

  const LibBDD::Call *map_put = get_future_map_put(node, map_objs.map);
  assert(map_put && "map_put not found");

  hh_table_data_t table_data(map_put);

  klee::ref<klee::Expr> min_estimate_cond = build_min_estimate_check_cond(ep, min_estimate, map_objs.map);

  LibCore::Symbols symbols = get_dataplane_state(ep, node);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  LibBDD::Branch *min_estimate_cond_branch;
  std::unique_ptr<LibBDD::BDD> new_bdd =
      rebuild_bdd(new_ep, dchain_allocate_new_index, map_objs, table_data.key, min_estimate_cond, min_estimate_cond_branch);

  Module *if_module                 = new If(min_estimate_cond_branch, min_estimate_cond, {min_estimate_cond});
  Module *then_module               = new Then(min_estimate_cond_branch);
  Module *else_module               = new Else(min_estimate_cond_branch);
  Module *send_to_controller_module = new SendToController(min_estimate_cond_branch->get_on_true(), symbols);
  Module *hh_table_update_module    = new Controller::HHTableUpdate(min_estimate_cond_branch->get_on_true(), table_data.obj,
                                                                    table_data.table_keys, table_data.value, min_estimate);

  EPNode *if_node                 = new EPNode(if_module);
  EPNode *then_node               = new EPNode(then_module);
  EPNode *else_node               = new EPNode(else_module);
  EPNode *send_to_controller_node = new EPNode(send_to_controller_module);
  EPNode *hh_table_update_node    = new EPNode(hh_table_update_module);

  if_node->set_constraint(min_estimate_cond);
  if_node->set_children(then_node, else_node);

  then_node->set_prev(if_node);
  then_node->set_children(send_to_controller_node);

  else_node->set_prev(if_node);

  send_to_controller_node->set_prev(then_node);
  send_to_controller_node->set_children(hh_table_update_node);

  hh_table_update_node->set_prev(send_to_controller_node);

  u32 capacity                  = ep->get_ctx().get_map_config(map_objs.map).capacity;
  hit_rate_t new_hh_probability = ep->get_ctx().get_profiler().get_bdd_profile()->churn_hit_rate_top_k_flows(map_objs.map, capacity);

  new_ep->get_mutable_ctx().get_mutable_profiler().insert_relative(new_ep->get_active_leaf().node->get_constraints(), min_estimate_cond,
                                                                   new_hh_probability);

  std::vector<klee::ref<klee::Expr>> lhs_cnstrs = node->get_ordered_branch_constraints();
  lhs_cnstrs.push_back(min_estimate_cond);
  lhs_cnstrs.push_back(index_alloc_check.branch->get_condition());

  std::vector<klee::ref<klee::Expr>> rhs_cnstrs = node->get_ordered_branch_constraints();
  rhs_cnstrs.push_back(LibCore::solver_toolbox.exprBuilder->Not(min_estimate_cond));
  rhs_cnstrs.push_back(LibCore::solver_toolbox.exprBuilder->Not(index_alloc_check.branch->get_condition()));

  new_ep->get_mutable_ctx().get_mutable_profiler().remove(lhs_cnstrs);
  new_ep->get_mutable_ctx().get_mutable_profiler().remove(rhs_cnstrs);

  EPLeaf on_hh_leaf(hh_table_update_node, min_estimate_cond_branch->get_on_true());
  EPLeaf on_not_hh_leaf(else_node, min_estimate_cond_branch->get_on_false());

  new_ep->process_leaf(if_node, {on_hh_leaf, on_not_hh_leaf});
  new_ep->replace_bdd(std::move(new_bdd));
  new_ep->assert_integrity();

  hit_rate_t hr = new_ep->get_ctx().get_profiler().get_hr(send_to_controller_node);
  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_traffic(new_ep->get_node_egress(hr, send_to_controller_node));

  return impls;
}

std::unique_ptr<Module> HHTableConditionalUpdateFactory::create(const LibBDD::BDD *bdd, const Context &ctx,
                                                                const LibBDD::Node *node) const {
  return {};
}

} // namespace Tofino
} // namespace LibSynapse