#include <LibSynapse/Modules/Controller/RotateLeft.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

namespace {

bool matches(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }
  return dynamic_cast<const Call *>(node)->get_call().function_name == "rotate_left";
}

} // namespace

std::optional<spec_impl_t> RotateLeftFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (!matches(node)) {
    return {};
  }
  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> RotateLeftFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!matches(node)) {
    return {};
  }

  const call_t &call = dynamic_cast<const Call *>(node)->get_call();

  Module *module  = new RotateLeft(node, call.args.at("x").expr, call.args.at("n").expr, call.ret);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);
  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> RotateLeftFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!matches(node)) {
    return {};
  }
  const call_t &call = dynamic_cast<const Call *>(node)->get_call();
  return std::make_unique<RotateLeft>(node, call.args.at("x").expr, call.args.at("n").expr, call.ret);
}

} // namespace Controller
} // namespace LibSynapse
