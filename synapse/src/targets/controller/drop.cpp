#include "drop.h"

namespace synapse {
namespace ctrl {

std::optional<spec_impl_t> DropFactory::speculate(const EP *ep, const Node *node, const Context &ctx) const {
  if (node->get_type() != NodeType::Route) {
    return std::nullopt;
  }

  const Route *route_node = dynamic_cast<const Route *>(node);
  RouteOp op              = route_node->get_operation();

  if (op != RouteOp::Drop) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.get_mutable_perf_oracle().add_dropped_traffic(new_ctx.get_profiler().get_hr(node));

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> DropFactory::process_node(const EP *ep, const Node *node, SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Route) {
    return impls;
  }

  const Route *route_node = dynamic_cast<const Route *>(node);
  RouteOp op              = route_node->get_operation();

  if (op != RouteOp::Drop) {
    return impls;
  }

  Module *module  = new Drop(node);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_dropped_traffic(
      new_ep->get_mutable_ctx().get_profiler().get_hr(node));

  return impls;
}

} // namespace ctrl
} // namespace synapse