#include <LibSynapse/Modules/Tofino/Broadcast.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

std::optional<spec_impl_t> BroadcastFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Route) {
    return std::nullopt;
  }

  const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(node);
  const LibBDD::RouteOp op        = route_node->get_operation();

  if (op != LibBDD::RouteOp::Broadcast) {
    return std::nullopt;
  }

  Context new_ctx                   = ctx;
  LibSynapse::fwd_stats_t fwd_stats = new_ctx.get_profiler().get_fwd_stats(node);
  assert(fwd_stats.operation == LibBDD::RouteOp::Broadcast);
  for (const auto &[device, _] : fwd_stats.ports) {
    new_ctx.get_mutable_perf_oracle().add_fwd_traffic(device, fwd_stats.flood);
  }

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> BroadcastFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  if (node->get_type() != LibBDD::NodeType::Route) {
    return {};
  }

  const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(node);
  const LibBDD::RouteOp op        = route_node->get_operation();

  if (op != LibBDD::RouteOp::Broadcast) {
    return {};
  }

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Module *module  = new Broadcast(node);
  EPNode *ep_node = new EPNode(module);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  LibSynapse::fwd_stats_t fwd_stats = new_ep->get_ctx().get_profiler().get_fwd_stats(node);
  assert(fwd_stats.operation == LibBDD::RouteOp::Broadcast);
  for (const auto &[device, _] : fwd_stats.ports) {
    new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_fwd_traffic(device, fwd_stats.flood);
  }

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> BroadcastFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Route) {
    return {};
  }

  const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(node);
  LibBDD::RouteOp op              = route_node->get_operation();

  if (op != LibBDD::RouteOp::Broadcast) {
    return {};
  }

  return std::make_unique<Broadcast>(node);
}

} // namespace Tofino
} // namespace LibSynapse