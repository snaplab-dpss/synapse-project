#include <LibSynapse/Modules/Controller/Drop.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> DropFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Route) {
    return std::nullopt;
  }

  const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(node);
  LibBDD::RouteOp op              = route_node->get_operation();

  if (op != LibBDD::RouteOp::Drop) {
    return std::nullopt;
  }

  Context new_ctx                   = ctx;
  LibSynapse::fwd_stats_t fwd_stats = new_ctx.get_profiler().get_fwd_stats(node);
  assert(fwd_stats.is_drop_only());
  new_ctx.get_mutable_perf_oracle().add_dropped_traffic(fwd_stats.drop);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> DropFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Route) {
    return impls;
  }

  const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(node);
  LibBDD::RouteOp op              = route_node->get_operation();

  if (op != LibBDD::RouteOp::Drop) {
    return impls;
  }

  Module *module  = new Drop(node);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  LibSynapse::fwd_stats_t fwd_stats = new_ep->get_ctx().get_profiler().get_fwd_stats(node);
  assert(fwd_stats.is_drop_only());
  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_dropped_traffic(fwd_stats.drop);

  return impls;
}

} // namespace Controller
} // namespace LibSynapse