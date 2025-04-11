#include <LibSynapse/Modules/Tofino/Drop.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

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
  return (type == ModuleType::Tofino_ParserCondition);
}

} // namespace

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
  assert(fwd_stats.operation == LibBDD::RouteOp::Drop);
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

  if (is_parser_reject(ep)) {
    return impls;
  }

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Module *module  = new Drop(node);
  EPNode *ep_node = new EPNode(module);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  LibSynapse::fwd_stats_t fwd_stats = new_ep->get_ctx().get_profiler().get_fwd_stats(node);
  assert(fwd_stats.operation == LibBDD::RouteOp::Drop);
  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_dropped_traffic(fwd_stats.drop);

  return impls;
}

std::unique_ptr<Module> DropFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Route) {
    return {};
  }

  const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(node);
  LibBDD::RouteOp op              = route_node->get_operation();

  if (op != LibBDD::RouteOp::Drop) {
    return {};
  }

  return std::make_unique<Drop>(node);
}

} // namespace Tofino
} // namespace LibSynapse