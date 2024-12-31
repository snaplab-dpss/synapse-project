#include "forward.h"

namespace tofino_cpu {

std::optional<spec_impl_t>
ForwardGenerator::speculate(const EP *ep, const Node *node,
                            const Context &ctx) const {
  if (node->get_type() != NodeType::Route) {
    return std::nullopt;
  }

  const Route *route_node = static_cast<const Route *>(node);
  RouteOp op = route_node->get_operation();

  if (op != RouteOp::Forward) {
    return std::nullopt;
  }

  int dst_device = route_node->get_dst_device();

  Context new_ctx = ctx;
  new_ctx.get_mutable_perf_oracle().add_fwd_traffic(
      dst_device, new_ctx.get_profiler().get_hr(node));

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> ForwardGenerator::process_node(const EP *ep,
                                                   const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Route) {
    return impls;
  }

  const Route *route_node = static_cast<const Route *>(node);
  RouteOp op = route_node->get_operation();

  if (op != RouteOp::Forward) {
    return impls;
  }

  int dst_device = route_node->get_dst_device();

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Module *module = new Forward(node, dst_device);
  EPNode *fwd_node = new EPNode(module);

  EPLeaf leaf(fwd_node, node->get_next());
  new_ep->process_leaf(fwd_node, {leaf});

  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_fwd_traffic(
      dst_device, get_node_egress(new_ep, fwd_node));

  return impls;
}

} // namespace tofino_cpu
