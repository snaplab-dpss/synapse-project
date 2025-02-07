#include <LibSynapse/Modules/x86/Forward.h>
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

  if (op != LibBDD::RouteOp::Forward) {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> ForwardFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> ForwardFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (!bdd_node_match_pattern(node)) {
    return impls;
  }

  const LibBDD::Route *route_node  = dynamic_cast<const LibBDD::Route *>(node);
  klee::ref<klee::Expr> dst_device = route_node->get_dst_device();

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Module *module  = new Forward(node, dst_device);
  EPNode *ep_node = new EPNode(module);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace x86
} // namespace LibSynapse