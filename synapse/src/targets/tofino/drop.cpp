#include "drop.h"

namespace tofino {

std::optional<spec_impl_t> DropGenerator::speculate(const EP *ep,
                                                    const Node *node,
                                                    const Context &ctx) const {
  if (node->get_type() != NodeType::Route) {
    return std::nullopt;
  }

  const Route *route_node = static_cast<const Route *>(node);
  RouteOp op = route_node->get_operation();

  if (op != RouteOp::Drop) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.get_mutable_perf_oracle().add_dropped_traffic(
      new_ctx.get_profiler().get_hr(node));

  return spec_impl_t(decide(ep, node), new_ctx);
}

bool is_parser_reject(const EP *ep) {
  EPLeaf leaf = ep->get_active_leaf();

  if (!leaf.node || !leaf.node->get_prev()) {
    return false;
  }

  const EPNode *node = leaf.node;
  const EPNode *prev = node->get_prev();
  const Module *module = prev->get_module();

  ModuleType type = module->get_type();
  return (type == ModuleType::Tofino_ParserCondition);
}

std::vector<impl_t> DropGenerator::process_node(const EP *ep,
                                                const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Route) {
    return impls;
  }

  const Route *route_node = static_cast<const Route *>(node);
  RouteOp op = route_node->get_operation();

  if (op != RouteOp::Drop) {
    return impls;
  }

  if (is_parser_reject(ep)) {
    return impls;
  }

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Module *module = new Drop(node);
  EPNode *ep_node = new EPNode(module);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.get_mutable_perf_oracle().add_dropped_traffic(
      ctx.get_profiler().get_hr(node));

  return impls;
}

} // namespace tofino
