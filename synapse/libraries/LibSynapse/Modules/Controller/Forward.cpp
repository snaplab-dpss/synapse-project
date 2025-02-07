#include <LibSynapse/Modules/Controller/Forward.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> ForwardFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Route) {
    return std::nullopt;
  }

  const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(node);
  LibBDD::RouteOp op              = route_node->get_operation();

  if (op != LibBDD::RouteOp::Forward) {
    return std::nullopt;
  }

  klee::ref<klee::Expr> dst_device = route_node->get_dst_device();

  Context new_ctx = ctx;
  assert(false && "TODO: grab this from the profiler");
  // new_ctx.get_mutable_perf_oracle().add_fwd_traffic(dst_device, new_ctx.get_profiler().get_hr(node));

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> ForwardFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Route) {
    return impls;
  }

  const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(node);
  LibBDD::RouteOp op              = route_node->get_operation();

  if (op != LibBDD::RouteOp::Forward) {
    return impls;
  }

  klee::ref<klee::Expr> dst_device = route_node->get_dst_device();

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Module *module   = new Forward(node, dst_device);
  EPNode *fwd_node = new EPNode(module);

  EPLeaf leaf(fwd_node, node->get_next());
  new_ep->process_leaf(fwd_node, {leaf});

  assert(false && "TODO: grab this from the profiler");
  // new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_fwd_traffic(dst_device, new_ep->get_node_egress(fwd_node));

  return impls;
}

} // namespace Controller
} // namespace LibSynapse