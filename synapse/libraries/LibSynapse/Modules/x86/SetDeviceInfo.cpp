#include <LibSynapse/Modules/x86/SetDeviceInfo.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace x86 {

using LibBDD::Call;
using LibBDD::call_t;

namespace {
bool bdd_node_match_pattern(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "set_device_info") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> SetDeviceInfoFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), speculations.ctx);
  return {};
}

std::vector<impl_t> SetDeviceInfoFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> device_id_expr = call.args.at("value").expr;

  Module *module  = new SetDeviceInfo(node, device_id_expr);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> SetDeviceInfoFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> device_id_expr = call.args.at("value").expr;

  std::unique_ptr<Module> module = std::make_unique<SetDeviceInfo>(node, device_id_expr);
  return module;
}

} // namespace x86
} // namespace LibSynapse
