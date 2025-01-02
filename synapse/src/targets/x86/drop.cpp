#include "drop.h"

namespace synapse {
namespace x86 {

namespace {
bool bdd_node_match_pattern(const Node *node) {
  if (node->get_type() != NodeType::Route) {
    return false;
  }

  const Route *route_node = dynamic_cast<const Route *>(node);
  RouteOp op = route_node->get_operation();

  if (op != RouteOp::Drop) {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> DropFactory::speculate(const EP *ep, const Node *node,
                                                  const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return std::nullopt;
}

std::vector<impl_t> DropFactory::process_node(const EP *ep, const Node *node) const {
  std::vector<impl_t> impls;

  if (!bdd_node_match_pattern(node)) {
    return impls;
  }

  Module *module = new Drop(node);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace x86
} // namespace synapse