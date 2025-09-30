#include <LibSynapse/Modules/x86/SendToDevice.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibBDD/Visitors/BDDVisualizer.h>
#include <LibSynapse/Visualizers/EPVisualizer.h>

namespace LibSynapse {
namespace x86 {
using LibBDD::Call;
using LibBDD::call_t;
namespace {

std::unique_ptr<BDD> replicate_hdr_parsing_ops(const EP *ep, const BDDNode *node, const BDDNode *&next) {
  std::vector<const Call *> prev_borrows = node->get_prev_functions({"packet_borrow_next_chunk"}, ep->get_target_roots(ep->get_active_target()));
  std::vector<const Call *> prev_returns = node->get_prev_functions({"packet_return_chunk"}, ep->get_target_roots(ep->get_active_target()));

  std::vector<const BDDNode *> hdr_parsing_ops;
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_borrows.begin(), prev_borrows.end());
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_returns.begin(), prev_returns.end());

  if (hdr_parsing_ops.empty()) {
    return nullptr;
  }

  const BDD *old_bdd = ep->get_bdd();

  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);
  new_bdd->add_cloned_non_branches(node->get_id(), hdr_parsing_ops);

  next = new_bdd->get_node_by_id(node->get_id());

  return new_bdd;
}
} // namespace

std::optional<spec_impl_t> SendToDeviceFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  Context new_ctx = ctx;

  // Don't send to the device if the node is already a route.
  if (node->get_type() == BDDNodeType::Route) {
    return {};
  }

  const hit_rate_t hr = new_ctx.get_profiler().get_hr(node);
  new_ctx.get_mutable_perf_oracle().add_controller_traffic(hr);

  // We can always send to the device, at any point in time.
  spec_impl_t spec_impl(decide(ep, node), new_ctx);
  spec_impl.next_target = TargetType(TargetArchitecture::x86, get_type().instance_id);

  return {};
}

std::vector<impl_t> SendToDeviceFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  const EPLeaf active_leaf = ep->get_active_leaf();

  // We can't send to the device if a forwarding decision was already made.
  if (active_leaf.node && active_leaf.node->forwarding_decision_already_made()) {
    return {};
  }

  // Don't send to the device if the node is already a route.
  if (node->get_type() == BDDNodeType::Route) {
    return {};
  }

  // Otherwise we can always send to the device, at any point in time.
  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Symbols symbols = get_relevant_dataplane_state(ep, node, get_target());
  symbols.remove("packet_chunks");

  // We need to decide the outgoing port and the incoming port.
  // The outgoing port is the port that leads to the next target.
  // The incoming port is the port that leads back to us from the next target.

  TargetType next_type = ep->get_placement(node->get_id());

  if (ep->get_active_target() == next_type)
    return {};

  u32 outgoing_port = ep->get_forwarding_port(ep->get_active_target().instance_id, next_type.instance_id);
  u32 incoming_port = 0;

  Module *module   = new SendToDevice(get_type().instance_id, node, outgoing_port, incoming_port, next_type, symbols);
  EPNode *s2d_node = new EPNode(module);

  EPNode *ep_node_leaf = s2d_node;

  const BDDNode *next          = node;
  std::unique_ptr<BDD> new_bdd = replicate_hdr_parsing_ops(ep, node, next);

  EPLeaf leaf(ep_node_leaf, next);
  new_ep->process_leaf(s2d_node, {leaf}, false);

  if (new_bdd) {
    new_ep->replace_bdd(std::move(new_bdd));
  }

  // const hit_rate_t hr = new_ep->get_ctx().get_profiler().get_hr(s2d_node);
  //  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_traffic(new_ep->get_node_egress(hr, s2d_node));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return {};
}

std::unique_ptr<Module> SendToDeviceFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  // We don't actually create a module for recirculation.
  return {};
}

} // namespace x86
} // namespace LibSynapse