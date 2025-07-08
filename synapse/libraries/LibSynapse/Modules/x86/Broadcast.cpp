#include <LibSynapse/Modules/x86/Broadcast.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace x86 {

namespace {
bool bdd_node_match_pattern(const LibBDD::Node *node) {
  if (node->get_type() != LibBDD::NodeType::Route) {
    return false;
  }

  const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(node);
  LibBDD::RouteOp op              = route_node->get_operation();

  if (op != LibBDD::RouteOp::Broadcast) {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> BroadcastFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return std::nullopt;
}

std::vector<impl_t> BroadcastFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  Module *module  = new Broadcast(node);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> BroadcastFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  return std::make_unique<Broadcast>(node);
}

} // namespace x86
} // namespace LibSynapse