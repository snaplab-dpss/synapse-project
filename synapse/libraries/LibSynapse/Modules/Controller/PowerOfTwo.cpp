#include <LibSynapse/Modules/Controller/PowerOfTwo.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

std::optional<spec_impl_t> PowerOfTwoFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }
  const Call *call_node = dynamic_cast<const Call *>(node);
  if (call_node->get_call().function_name != "power_of_two") {
    return {};
  }
  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> PowerOfTwoFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }
  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();
  if (call.function_name != "power_of_two") {
    return {};
  }

  Module *module  = new PowerOfTwo(node, call.args.at("exponent").expr, call.ret);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);
  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> PowerOfTwoFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }
  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();
  if (call.function_name != "power_of_two") {
    return {};
  }
  return std::make_unique<PowerOfTwo>(node, call.args.at("exponent").expr, call.ret);
}

} // namespace Controller
} // namespace LibSynapse
