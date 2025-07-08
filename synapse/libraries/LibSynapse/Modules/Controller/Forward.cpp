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

  Context new_ctx                   = ctx;
  LibSynapse::fwd_stats_t fwd_stats = new_ctx.get_profiler().get_fwd_stats(node);
  assert(fwd_stats.operation == LibBDD::RouteOp::Forward);
  for (const auto &[device, dev_hr] : fwd_stats.ports) {
    new_ctx.get_mutable_perf_oracle().add_fwd_traffic(device, dev_hr);
  }

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> ForwardFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  if (node->get_type() != LibBDD::NodeType::Route) {
    return {};
  }

  const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(node);
  const LibBDD::RouteOp op        = route_node->get_operation();

  if (op != LibBDD::RouteOp::Forward) {
    return {};
  }

  klee::ref<klee::Expr> dst_device = route_node->get_dst_device();

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Module *module   = new Forward(node, dst_device);
  EPNode *fwd_node = new EPNode(module);

  EPLeaf leaf(fwd_node, node->get_next());
  new_ep->process_leaf(fwd_node, {leaf});

  LibSynapse::fwd_stats_t fwd_stats = new_ep->get_ctx().get_profiler().get_fwd_stats(node);
  assert(fwd_stats.operation == LibBDD::RouteOp::Forward);
  for (const auto &[device, dev_hr] : fwd_stats.ports) {
    new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_fwd_traffic(device, new_ep->get_node_egress(dev_hr, fwd_node));
  }

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> ForwardFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Route) {
    return {};
  }

  const LibBDD::Route *route_node  = dynamic_cast<const LibBDD::Route *>(node);
  LibBDD::RouteOp op               = route_node->get_operation();
  klee::ref<klee::Expr> dst_device = route_node->get_dst_device();

  if (op != LibBDD::RouteOp::Forward) {
    return {};
  }

  return std::make_unique<Forward>(node, dst_device);
}

} // namespace Controller
} // namespace LibSynapse