#include <LibSynapse/Modules/Tofino/Broadcast.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Route;
using LibBDD::RouteOp;

std::optional<spec_impl_t> BroadcastFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Route) {
    return {};
  }

  const Route *route_node = dynamic_cast<const Route *>(node);
  const RouteOp op        = route_node->get_operation();

  if (op != RouteOp::Broadcast) {
    return {};
  }

  Context new_ctx             = ctx;
  const fwd_stats_t fwd_stats = new_ctx.get_profiler().get_fwd_stats(node);
  assert(fwd_stats.operation == RouteOp::Broadcast);

  const EPNode *leaf_node = ep->get_leaf_ep_node_from_bdd_node(node);

  for (const auto &[device, _] : fwd_stats.ports) {
    port_ingress_t node_egress;
    if (leaf_node) {
      node_egress = ep->get_node_egress(fwd_stats.flood, leaf_node);
    } else {
      node_egress.global = fwd_stats.flood;
    }
    new_ctx.get_mutable_perf_oracle().add_fwd_traffic(device, node_egress);
  }

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> BroadcastFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Route) {
    return {};
  }

  const Route *route_node = dynamic_cast<const Route *>(node);
  const RouteOp op        = route_node->get_operation();

  if (op != RouteOp::Broadcast) {
    return {};
  }

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Module *module  = new Broadcast(get_type().instance_id, node);
  EPNode *ep_node = new EPNode(module);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  const fwd_stats_t fwd_stats = new_ep->get_ctx().get_profiler().get_fwd_stats(node);
  assert(fwd_stats.operation == RouteOp::Broadcast);
  for (const auto &[device, _] : fwd_stats.ports) {
    new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_fwd_traffic(device, fwd_stats.flood);
  }

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> BroadcastFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Route) {
    return {};
  }

  const Route *route_node = dynamic_cast<const Route *>(node);
  const RouteOp op        = route_node->get_operation();

  if (op != RouteOp::Broadcast) {
    return {};
  }

  return std::make_unique<Broadcast>(get_type().instance_id, node);
}

} // namespace Tofino
} // namespace LibSynapse