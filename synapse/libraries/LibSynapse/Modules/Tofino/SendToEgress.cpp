#include <LibSynapse/Modules/Tofino/SendToEgress.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

std::optional<spec_impl_t> SendToEgressFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  // Nothing to predict: crossing into egress implements no BDD node.
  return {};
}

std::vector<impl_t> SendToEgressFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  const EPLeaf active_leaf = ep->get_active_leaf();

  if (!active_leaf.node) {
    return {};
  }

  // The egress cannot choose a port: ucast_egress_port is written in ingress. So the crossing is
  // only legal once the forwarding decision is fixed, which is what "pulling the decision back"
  // means. BDD reordering is what creates those opportunities, by hoisting the route node above
  // the work that is left.
  if (!active_leaf.node->forwarding_decision_already_made()) {
    return {};
  }

  // One crossing per pass: there is no way back to ingress without recirculating, and a
  // recirculation needs a port, which egress cannot set.
  const EPNode *prev = active_leaf.node;
  while (prev) {
    const Module *prev_module = prev->get_module();
    if (prev_module && prev_module->get_type() == ModuleType::Tofino_SendToEgress) {
      return {};
    }
    prev = prev->get_prev();
  }

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  // Deliberately no perf oracle update: unlike a recirculation this costs no extra pass through
  // the traffic manager.

  const Symbols symbols = get_relevant_dataplane_state(ep, node);

  Module *module  = new SendToEgress(node, symbols);
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
