#include <LibSynapse/Modules/Controller/Forward.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Route;
using LibBDD::RouteOp;

std::optional<spec_impl_t> ForwardFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Route) {
    return {};
  }

  const Route *route_node = dynamic_cast<const Route *>(node);
  const RouteOp op        = route_node->get_operation();

  if (op != RouteOp::Forward) {
    return {};
  }

  Context new_ctx = ctx;

  const fwd_stats_t fwd_stats                       = ctx.get_profiler().get_fwd_stats(node);
  const std::unordered_set<u16> candidate_fwd_ports = ctx.get_profiler().get_candidate_fwd_ports(node);
  assert(fwd_stats.operation == RouteOp::Forward);
  for (const u16 device : candidate_fwd_ports) {
    const hit_rate_t dev_hr = fwd_stats.ports.at(device);
    if (dev_hr == 0_hr) {
      continue;
    }

    port_ingress_t node_egress;
    node_egress.controller = dev_hr;

    new_ctx.get_mutable_perf_oracle().add_fwd_traffic(device, node_egress);
  }

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> ForwardFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Route) {
    return {};
  }

  const Route *route_node = dynamic_cast<const Route *>(node);
  const RouteOp op        = route_node->get_operation();

  if (op != RouteOp::Forward) {
    return {};
  }

  klee::ref<klee::Expr> dst_device = route_node->get_dst_device();

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Module *module   = new Forward(ep->get_placement(node->get_id()), node, dst_device);
  EPNode *fwd_node = new EPNode(module);

  const EPLeaf leaf(fwd_node, node->get_next());
  new_ep->process_leaf(fwd_node, {leaf});

  const fwd_stats_t fwd_stats                       = new_ep->get_ctx().get_profiler().get_fwd_stats(node);
  const std::unordered_set<u16> candidate_fwd_ports = new_ep->get_ctx().get_profiler().get_candidate_fwd_ports(node);
  assert(fwd_stats.operation == RouteOp::Forward);
  for (const u16 device : candidate_fwd_ports) {
    const hit_rate_t dev_hr = fwd_stats.ports.at(device);
    if (dev_hr == 0_hr) {
      continue;
    }

    const port_ingress_t fwd_node_egress = new_ep->get_node_egress(dev_hr, fwd_node);
    new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_fwd_traffic(device, fwd_node_egress);
  }

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> ForwardFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Route) {
    return {};
  }

  const Route *route_node          = dynamic_cast<const Route *>(node);
  const RouteOp op                 = route_node->get_operation();
  klee::ref<klee::Expr> dst_device = route_node->get_dst_device();

  if (op != RouteOp::Forward) {
    return {};
  }

  return std::make_unique<Forward>(get_type().instance_id, node, dst_device);
}

} // namespace Controller
} // namespace LibSynapse