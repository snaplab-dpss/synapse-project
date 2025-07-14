#include <LibSynapse/Modules/Tofino/Drop.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Route;
using LibBDD::RouteOp;

namespace {

bool is_parser_reject(const EP *ep) {
  EPLeaf leaf = ep->get_active_leaf();

  if (!leaf.node || !leaf.node->get_prev()) {
    return false;
  }

  const EPNode *node   = leaf.node;
  const EPNode *prev   = node->get_prev();
  const Module *module = prev->get_module();

  return (module->get_type() == ModuleType::Tofino_ParserCondition);
}

} // namespace

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
  new_ctx.get_mutable_perf_oracle().add_dropped_traffic(fwd_stats.drop);

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

  if (is_parser_reject(ep)) {
    return {};
  }

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Module *module  = new Drop(node);
  EPNode *ep_node = new EPNode(module);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  const fwd_stats_t fwd_stats = new_ep->get_ctx().get_profiler().get_fwd_stats(node);
  assert(fwd_stats.operation == RouteOp::Drop);
  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_dropped_traffic(fwd_stats.drop);

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

  return std::make_unique<Drop>(node);
}

} // namespace Tofino
} // namespace LibSynapse