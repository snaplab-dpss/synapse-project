#include <LibSynapse/Modules/Tofino/GuardedMapTableGuardCheck.h>
#include <LibSynapse/Modules/Tofino/If.h>
#include <LibSynapse/Modules/Tofino/Then.h>
#include <LibSynapse/Modules/Tofino/Else.h>
#include <LibSynapse/Modules/Tofino/Forward.h>
#include <LibSynapse/Modules/Tofino/Drop.h>
#include <LibSynapse/Modules/Tofino/Broadcast.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Visualizers/EPVisualizer.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::BDDNodeManager;
using LibBDD::Branch;
using LibBDD::branch_direction_t;
using LibBDD::Call;
using LibBDD::call_t;
using LibBDD::Route;

using LibCore::expr_addr_to_obj_addr;

namespace {

symbol_t create_guard_check_symbol(DS_ID id, SymbolManager *symbol_manager, const BDDNode *node) {
  const std::string name = id + "_" + std::to_string(node->get_id());
  const bits_t size      = 8;
  return symbol_manager->create_symbol(name, size);
}

klee::ref<klee::Expr> build_guard_allow_condition(const symbol_t &guard_check_symbol) {
  const bits_t width               = guard_check_symbol.expr->getWidth();
  const klee::ref<klee::Expr> zero = solver_toolbox.exprBuilder->Constant(0, width);
  return solver_toolbox.exprBuilder->Ne(guard_check_symbol.expr, zero);
}

bool guarded_map_table_guard_check_already_performed(const EP *ep, addr_t obj) {
  const EPLeaf active_leaf = ep->get_active_leaf();
  const EPNode *node       = active_leaf.node;

  while (node) {
    const Module *module = node->get_module();
    assert(module);

    if (module->get_type().type == ModuleCategory::Tofino_GuardedMapTableGuardCheck) {
      const GuardedMapTableGuardCheck *guarded_map_table_guard_check = dynamic_cast<const GuardedMapTableGuardCheck *>(module);
      if (guarded_map_table_guard_check->get_obj() == obj) {
        return true;
      }
    }

    node = node->get_prev();
  }

  return false;
}

double get_guard_allow_probability(const EP *ep, const branch_direction_t &branch_checking_dchain_index_alloc) {
  const Profiler &profiler = ep->get_ctx().get_profiler();

  const Branch *branch      = branch_checking_dchain_index_alloc.branch;
  const BDDNode *on_success = branch_checking_dchain_index_alloc.get_success_node();

  const hit_rate_t index_alloc_hr         = profiler.get_hr(branch);
  const hit_rate_t index_alloc_success_hr = profiler.get_hr(on_success);

  const double guard_allow_probability = index_alloc_success_hr / index_alloc_hr;

  return guard_allow_probability;
}

struct new_bdd_nodes_t {
  const Branch *guard_check_branch;
  branch_direction_t index_alloc_check_on_guard_allow;
  std::vector<klee::ref<klee::Expr>> success_index_alloc_on_guard_disallow_constraints;
};

std::unique_ptr<BDD> rebuild_bdd(const EP *ep, const Call *dchain_allocate_new_index, const symbol_t &guard_check_symbol,
                                 klee::ref<klee::Expr> guard_allow_condition, new_bdd_nodes_t &new_bdd_nodes) {
  const BDD *old_bdd           = ep->get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  new_bdd->add_new_symbol_generator_function(dchain_allocate_new_index->get_id(), "guarded_map_table_guard_check", Symbols({guard_check_symbol}));
  new_bdd_nodes.guard_check_branch = new_bdd->add_cloned_branch(dchain_allocate_new_index->get_id(), guard_allow_condition);

  // When not allowing, let's use the branch side of an unsuccessful dchain allocation (and also remove the index allocation).
  const Call *on_guard_allow    = dynamic_cast<const Call *>(new_bdd_nodes.guard_check_branch->get_on_true());
  const Call *on_guard_disallow = dynamic_cast<const Call *>(new_bdd_nodes.guard_check_branch->get_on_false());

  assert(on_guard_allow->get_type() == BDDNodeType::Call);
  assert(on_guard_disallow->get_type() == BDDNodeType::Call);
  assert(on_guard_allow->get_call().function_name == "dchain_allocate_new_index");
  assert(on_guard_disallow->get_call().function_name == "dchain_allocate_new_index");

  new_bdd_nodes.index_alloc_check_on_guard_allow               = new_bdd->find_branch_checking_index_alloc(on_guard_allow);
  const branch_direction_t index_alloc_check_on_guard_disallow = new_bdd->find_branch_checking_index_alloc(on_guard_disallow);

  assert(new_bdd_nodes.index_alloc_check_on_guard_allow.branch);
  assert(index_alloc_check_on_guard_disallow.branch);

  new_bdd_nodes.success_index_alloc_on_guard_disallow_constraints =
      index_alloc_check_on_guard_disallow.get_success_node()->get_ordered_branch_constraints();

  const BDD::BranchDeletionAction branch_deletion_action =
      !index_alloc_check_on_guard_disallow.direction ? BDD::BranchDeletionAction::KeepOnTrue : BDD::BranchDeletionAction::KeepOnFalse;

  new_bdd->delete_non_branch(on_guard_disallow->get_id());
  new_bdd->delete_branch(index_alloc_check_on_guard_disallow.branch->get_id(), branch_deletion_action);

  return new_bdd;
}

bool update_profiler(EP *new_ep, double guard_allow_probability, const new_bdd_nodes_t &new_bdd_nodes) {
  Profiler &new_profiler = new_ep->get_mutable_ctx().get_mutable_profiler();

  new_profiler.insert_relative(new_bdd_nodes.guard_check_branch->get_ordered_branch_constraints(), new_bdd_nodes.guard_check_branch->get_condition(),
                               hit_rate_t{guard_allow_probability});
  new_profiler.remove(new_bdd_nodes.success_index_alloc_on_guard_disallow_constraints);

  if (!new_profiler.can_set(new_bdd_nodes.index_alloc_check_on_guard_allow.get_failure_node()->get_ordered_branch_constraints(), 0_hr)) {
    return false;
  }

  new_profiler.set(new_bdd_nodes.index_alloc_check_on_guard_allow.get_failure_node()->get_ordered_branch_constraints(), 0_hr);
  return true;
}

} // namespace

std::optional<spec_impl_t> GuardedMapTableGuardCheckFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!ep->get_bdd()->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts)) {
    return {};
  }

  assert(!future_map_puts.empty());

  for (const Call *map_put : future_map_puts) {
    const addr_t obj = expr_addr_to_obj_addr(map_put->get_obj());
    if (!ctx.check_ds_impl(obj, DSImpl::Tofino_GuardedMapTable)) {
      return {};
    }
  }

  const branch_direction_t branch_direction = ep->get_bdd()->find_branch_checking_index_alloc(dchain_allocate_new_index);
  if (branch_direction.branch == nullptr) {
    return {};
  }

  const std::vector<const Route *> future_routing_decisions = branch_direction.get_failure_node()->get_future_routing_decisions();

  Context new_ctx = ctx;

  for (const Route *route : future_routing_decisions) {
    const fwd_stats_t fwd_stats = new_ctx.get_profiler().get_fwd_stats(route);
    assert(fwd_stats.operation == route->get_operation());
    switch (route->get_operation()) {
    case RouteOp::Forward: {
      for (const auto &[device, dev_hr] : fwd_stats.ports) {
        new_ctx.get_mutable_perf_oracle().add_fwd_traffic(device, dev_hr);
      }
    } break;
    case RouteOp::Drop: {
      new_ctx.get_mutable_perf_oracle().add_dropped_traffic(fwd_stats.drop);
    } break;
    case RouteOp::Broadcast: {
      for (const auto &[device, _] : fwd_stats.ports) {
        new_ctx.get_mutable_perf_oracle().add_fwd_traffic(device, fwd_stats.flood);
      }
    } break;
    }
  }

  new_ctx.get_mutable_profiler().set(branch_direction.get_failure_node()->get_ordered_branch_constraints(), 0_hr);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> GuardedMapTableGuardCheckFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!ep->get_bdd()->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts)) {
    return {};
  }

  assert(!future_map_puts.empty());

  for (const Call *map_put : future_map_puts) {
    const addr_t obj = expr_addr_to_obj_addr(map_put->get_obj());

    if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_GuardedMapTable)) {
      return {};
    }
  }

  const branch_direction_t branch_direction = ep->get_bdd()->find_branch_checking_index_alloc(dchain_allocate_new_index);
  if (branch_direction.branch == nullptr) {
    return {};
  }

  const addr_t obj = expr_addr_to_obj_addr(future_map_puts[0]->get_obj());

  if (guarded_map_table_guard_check_already_performed(ep, obj)) {
    return {};
  }

  const GuardedMapTable *guarded_map_table = get_tofino_ctx(ep)->get_data_structures().get_single_ds<GuardedMapTable>(obj);
  const symbol_t guard_check_symbol        = create_guard_check_symbol(guarded_map_table->id, symbol_manager, node);

  const double guard_allow_probability              = get_guard_allow_probability(ep, branch_direction);
  const klee::ref<klee::Expr> guard_allow_condition = build_guard_allow_condition(guard_check_symbol);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  new_bdd_nodes_t new_bdd_nodes;
  std::unique_ptr<BDD> new_bdd = rebuild_bdd(new_ep.get(), dchain_allocate_new_index, guard_check_symbol, guard_allow_condition, new_bdd_nodes);
  if (!update_profiler(new_ep.get(), guard_allow_probability, new_bdd_nodes)) {
    return {};
  }

  Module *guard_check_module = new GuardedMapTableGuardCheck(node, guarded_map_table->id, obj, guard_check_symbol, guard_allow_condition);
  Module *if_module          = new If(node, guard_allow_condition);
  Module *then_module        = new Then(node);
  Module *else_module        = new Else(node);

  EPNode *guard_check_ep_node = new EPNode(guard_check_module);
  EPNode *if_ep_node          = new EPNode(if_module);
  EPNode *then_ep_node        = new EPNode(then_module);
  EPNode *else_ep_node        = new EPNode(else_module);

  guard_check_ep_node->set_children(if_ep_node);
  if_ep_node->set_prev(guard_check_ep_node);

  if_ep_node->set_children(guard_allow_condition, then_ep_node, else_ep_node);
  then_ep_node->set_prev(if_ep_node);
  else_ep_node->set_prev(if_ep_node);

  EPLeaf then_leaf(then_ep_node, new_bdd_nodes.guard_check_branch->get_on_true());
  EPLeaf else_leaf(else_ep_node, new_bdd_nodes.guard_check_branch->get_on_false());
  new_ep->process_leaf(guard_check_ep_node, {then_leaf, else_leaf});

  new_ep->replace_bdd(std::move(new_bdd));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> GuardedMapTableGuardCheckFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!bdd->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts)) {
    return {};
  }

  assert(!future_map_puts.empty());

  for (const Call *map_put : future_map_puts) {
    const addr_t obj = expr_addr_to_obj_addr(map_put->get_obj());

    if (!ctx.check_ds_impl(obj, DSImpl::Tofino_GuardedMapTable)) {
      return {};
    }
  }

  const addr_t obj                         = expr_addr_to_obj_addr(future_map_puts[0]->get_obj());
  const GuardedMapTable *guarded_map_table = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_single_ds<GuardedMapTable>(obj);

  const symbol_t mock_guard_symbol;
  klee::ref<klee::Expr> mock_condition;

  return std::make_unique<GuardedMapTableGuardCheck>(type, node, guarded_map_table->id, obj, mock_guard_symbol, mock_condition);
}

} // namespace Tofino
} // namespace LibSynapse
