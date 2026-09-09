#include <LibSynapse/Modules/Tofino/SendToEgress.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Solver.h>

namespace LibSynapse {
namespace Tofino {

namespace {
// Crossing into egress is only worth considering once the ingress has real work in it.
constexpr size_t MIN_MODULES_BEFORE_EGRESS_CROSSING = 8;

// What the egress can actually implement. Everything past the cut has to be one of these, or the
// plan reaches a point with no legal module and the whole search dead-ends: there is no way back
// to ingress, no recirculating and no handing off to the controller from there. Stateful calls
// are excluded because they become controller-managed tables, which sycon only addresses in the
// ingress.
bool implementable_in_egress(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return true;
  }

  const std::string &fname = static_cast<const LibBDD::Call *>(node)->get_call().function_name;
  // nf_set_rte_ipv4_udptcp_checksum is on the list because synapse ignores it outright (there is
  // no dataplane checksum yet), so it costs the egress nothing.
  return fname.rfind("op_", 0) == 0 || fname == "rotate_left" || fname == "packet_borrow_next_chunk" ||
         fname == "packet_return_chunk" || fname == "packet_state_total_length" || fname == "packet_get_unread_length";
}
} // namespace

std::optional<spec_impl_t> SendToEgressFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  // Nothing to predict: crossing into egress implements no BDD node.
  return {};
}

std::vector<impl_t> SendToEgressFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  const EPLeaf active_leaf = ep->get_active_leaf();

  if (!active_leaf.node) {
    return {};
  }

  // The egress cannot choose a port: ucast_egress_port is written in ingress. Every BDD path
  // ends in its route node, so waiting for the decision to have been made leaves nothing to do
  // on the far side; instead the decision is pulled back to here, which is only sound when every
  // route still reachable agrees on it. Drops are fine, egress can drop; broadcast is not.
  bool uniform_route = true;
  bool saw_forward   = false;
  bool work_remains  = false;
  klee::ref<klee::Expr> dst_device;

  bool all_implementable = true;

  node->visit_nodes([&](const BDDNode *future) {
    if (future->get_type() != BDDNodeType::Route) {
      work_remains = true;
      if (!implementable_in_egress(future)) {
        all_implementable = false;
        return BDDNodeVisitAction::Stop;
      }
      return BDDNodeVisitAction::Continue;
    }

    const LibBDD::Route *route = dynamic_cast<const LibBDD::Route *>(future);
    switch (route->get_operation()) {
    case LibBDD::RouteOp::Drop:
      break;
    case LibBDD::RouteOp::Broadcast:
      uniform_route = false;
      break;
    case LibBDD::RouteOp::Forward: {
      const klee::ref<klee::Expr> device = route->get_dst_device();
      if (!saw_forward) {
        saw_forward = true;
        dst_device  = device;
      } else if (!solver_toolbox.are_exprs_always_equal(dst_device, device)) {
        uniform_route = false;
      }
      break;
    }
    }

    return uniform_route ? BDDNodeVisitAction::Continue : BDDNodeVisitAction::Stop;
  });

  if (!uniform_route) {
    return {};
  }
  if (!work_remains) {
    return {};
  }
  if (!all_implementable) {
    return {};
  }

  // One crossing per pass: there is no way back to ingress without recirculating, and a
  // recirculation needs a port, which egress cannot set. The same walk counts what this pass has
  // already placed: crossing buys depth, so offering it before the ingress holds any real work
  // only multiplies the search space, which it does violently (2e61 estimated states against the
  // 2e11 of a plain ingress search).
  size_t modules_this_pass = 0;
  for (const EPNode *prev = active_leaf.node; prev; prev = prev->get_prev()) {
    const Module *prev_module = prev->get_module();
    if (!prev_module || prev_module->get_target() != TargetType::Tofino) {
      break;
    }
    if (prev_module->get_type() == ModuleType::Tofino_SendToEgress) {
        return {};
    }
    if (prev_module->get_type() == ModuleType::Tofino_Recirculate) {
      break;
    }
    modules_this_pass++;
  }

  if (modules_this_pass < MIN_MODULES_BEFORE_EGRESS_CROSSING) {
    return {};
  }


  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  // Deliberately no perf oracle update: unlike a recirculation this costs no extra pass through
  // the traffic manager.

  const Symbols symbols = get_relevant_dataplane_state(ep, node);

  Module *module  = new SendToEgress(node, symbols, dst_device);
  EPNode *ep_node = new EPNode(module);

  // The node itself is not implemented here, only moved to the other pipeline.
  const EPLeaf leaf(ep_node, node);
  new_ep->process_leaf(ep_node, {leaf}, EP::BDDNodeMarking::Unprocessed);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));

  return impls;
}

std::unique_ptr<Module> SendToEgressFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  // Like recirculation, there is no standalone module to create.
  return {};
}

} // namespace Tofino
} // namespace LibSynapse
