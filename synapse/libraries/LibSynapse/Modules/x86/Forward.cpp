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

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> ForwardFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Route *route_node          = dynamic_cast<const Route *>(node);
  klee::ref<klee::Expr> dst_device = route_node->get_dst_device();

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Module *module  = new Forward(node, dst_device);
  EPNode *ep_node = new EPNode(module);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

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

  return std::make_unique<Forward>(node, dst_device);
}

} // namespace x86
} // namespace LibSynapse