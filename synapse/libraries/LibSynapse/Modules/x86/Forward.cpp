#include <LibSynapse/Modules/x86/Forward.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace x86 {

using LibBDD::Route;
using LibBDD::RouteOp;

using LibCore::expr_addr_to_obj_addr;

namespace {
bool bdd_node_match_pattern(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Route) {
    return false;
  }

  const Route *route_node = dynamic_cast<const Route *>(node);
  const RouteOp op        = route_node->get_operation();

  if (op != RouteOp::Forward) {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> ForwardFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  Context new_ctx             = ctx;
  const fwd_stats_t fwd_stats = new_ctx.get_profiler().get_fwd_stats(node);
  assert(fwd_stats.operation == RouteOp::Forward);
  for (const auto &[device, dev_hr] : fwd_stats.ports) {
    new_ctx.get_mutable_perf_oracle().add_fwd_traffic(device, dev_hr);
  }

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> ForwardFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Route *route_node          = dynamic_cast<const Route *>(node);
  klee::ref<klee::Expr> dst_device = route_node->get_dst_device();

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Module *module  = new Forward(get_type().instance_id, node, dst_device);
  EPNode *ep_node = new EPNode(module);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  const fwd_stats_t fwd_stats = new_ep->get_ctx().get_profiler().get_fwd_stats(node);
  assert(fwd_stats.operation == RouteOp::Forward);
  for (const auto &[device, dev_hr] : fwd_stats.ports) {
    const port_ingress_t fwd_node_egress = new_ep->get_node_egress(dev_hr, ep_node);
    new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_fwd_traffic(device, fwd_node_egress);
  }
  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> ForwardFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Route *route_node          = dynamic_cast<const Route *>(node);
  klee::ref<klee::Expr> dst_device = route_node->get_dst_device();

  return std::make_unique<Forward>(get_type().instance_id, node, dst_device);
}

} // namespace x86
} // namespace LibSynapse
