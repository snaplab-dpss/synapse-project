#include <LibSynapse/Modules/Tofino/ParserReject.h>
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

  ModuleType type = module->get_type();
  return (type.type == ModuleCategory::Tofino_ParserCondition);
}
} // namespace

std::optional<spec_impl_t> ParserRejectFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  // No need to speculate this.
  return {};
}

std::vector<impl_t> ParserRejectFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Route) {
    return {};
  }

  const Route *route_node = dynamic_cast<const Route *>(node);
  const RouteOp op        = route_node->get_operation();

  if (op != RouteOp::Drop) {
    return {};
  }

  if (!is_parser_reject(ep)) {
    return {};
  }

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Module *module  = new ParserReject(get_type().instance_id, node);
  EPNode *ep_node = new EPNode(module);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.get_mutable_perf_oracle().add_dropped_traffic(ctx.get_profiler().get_hr(node));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> ParserRejectFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Route) {
    return {};
  }

  const Route *route_node = dynamic_cast<const Route *>(node);
  const RouteOp op        = route_node->get_operation();

  if (op != RouteOp::Drop) {
    return {};
  }

  return std::make_unique<ParserReject>(get_type().instance_id, node);
}

} // namespace Tofino
} // namespace LibSynapse