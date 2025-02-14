#include <LibSynapse/Modules/Tofino/ParserReject.h>
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

std::optional<spec_impl_t> ParserRejectFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Route) {
    return std::nullopt;
  }

  const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(node);
  LibBDD::RouteOp op              = route_node->get_operation();

  if (op != LibBDD::RouteOp::Drop) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.get_mutable_perf_oracle().add_dropped_traffic(new_ctx.get_profiler().get_hr(node));

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> ParserRejectFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                      LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Route) {
    return impls;
  }

  const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(node);
  LibBDD::RouteOp op              = route_node->get_operation();

  if (op != LibBDD::RouteOp::Drop) {
    return impls;
  }

  if (!is_parser_reject(ep)) {
    return impls;
  }

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Module *module  = new ParserReject(node);
  EPNode *ep_node = new EPNode(module);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.get_mutable_perf_oracle().add_dropped_traffic(ctx.get_profiler().get_hr(node));

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->parser_reject(ep, node);

  return impls;
}

std::unique_ptr<Module> ParserRejectFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Route) {
    return {};
  }

  const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(node);
  LibBDD::RouteOp op              = route_node->get_operation();

  if (op != LibBDD::RouteOp::Drop) {
    return {};
  }

  return std::make_unique<ParserReject>(node);
}

} // namespace Tofino
} // namespace LibSynapse