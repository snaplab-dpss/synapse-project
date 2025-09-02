#include <LibSynapse/Modules/Controller/Drop.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Route;
using LibBDD::RouteOp;

std::optional<spec_impl_t> DropFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Route) {
    return {};
  }

  const Route *route_node = dynamic_cast<const Route *>(node);
  const RouteOp op        = route_node->get_operation();

  if (op != RouteOp::Drop) {
    return {};
  }

  Context new_ctx             = ctx;
  const fwd_stats_t fwd_stats = new_ctx.get_profiler().get_fwd_stats(node);
  assert(fwd_stats.operation == RouteOp::Drop);
  new_ctx.get_mutable_perf_oracle().add_controller_dropped_traffic(fwd_stats.drop);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> DropFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Route) {
    return {};
  }

  const Route *route_node = dynamic_cast<const Route *>(node);
  const RouteOp op        = route_node->get_operation();

  if (op != RouteOp::Drop) {
    return {};
  }

  Module *module  = new Drop(ep->get_placement(node->get_id()), node);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  const fwd_stats_t fwd_stats = new_ep->get_ctx().get_profiler().get_fwd_stats(node);
  assert(fwd_stats.operation == RouteOp::Drop);
  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_dropped_traffic(fwd_stats.drop);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DropFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Route) {
    return {};
  }

  const Route *route_node = dynamic_cast<const Route *>(node);
  const RouteOp op        = route_node->get_operation();

  if (op != RouteOp::Drop) {
    return {};
  }

  return std::make_unique<Drop>(get_type().instance_id, node);
}

} // namespace Controller
} // namespace LibSynapse