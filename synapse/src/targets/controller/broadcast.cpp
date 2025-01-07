#include "broadcast.h"

namespace synapse {
namespace ctrl {

std::optional<spec_impl_t> BroadcastFactory::speculate(const EP *ep, const Node *node,
                                                       const Context &ctx) const {
  if (node->get_type() != NodeType::Route) {
    return std::nullopt;
  }

  const Route *route_node = dynamic_cast<const Route *>(node);
  RouteOp op = route_node->get_operation();

  if (op != RouteOp::Broadcast) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> BroadcastFactory::process_node(const EP *ep, const Node *node,
                                                   SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Route) {
    return impls;
  }

  const Route *route_node = dynamic_cast<const Route *>(node);
  RouteOp op = route_node->get_operation();

  if (op != RouteOp::Broadcast) {
    return impls;
  }

  Module *module = new Broadcast(node);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace ctrl
} // namespace synapse