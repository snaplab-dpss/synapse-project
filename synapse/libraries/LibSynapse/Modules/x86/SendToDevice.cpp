#include <LibSynapse/Modules/x86/SendToDevice.h>
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

  const Call *call_op = dynamic_cast<const Call *>(node);
  const call_t call   = call_op->get_call();

  if (call.function_name != "send_to_device") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> SendToDeviceFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  Context new_ctx = speculations.ctx;

  // Don't send to the device if the node is already a route.
  if (node->get_type() == BDDNodeType::Route) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> next_target_expr = call.args.at("next_target").expr;

  spec_impl_t spec_impl(decide(ep, node), new_ctx);

  return spec_impl;
}

std::vector<impl_t> SendToDeviceFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> outgoing_port    = call.args.at("outgoing_port").expr;
  klee::ref<klee::Expr> next_target_expr = call.args.at("next_target").expr;
  klee::ref<klee::Expr> code_path        = call.args.at("code_path").expr;

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);
  Symbols symbols            = call_node->get_local_symbols();

  Module *module   = new SendToDevice(node, outgoing_port, code_path, symbols);
  EPNode *s2d_node = new EPNode(module);

  EPNode *ep_node_leaf = s2d_node;

  EPLeaf leaf(ep_node_leaf, node->get_next());
  new_ep->process_leaf(s2d_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> SendToDeviceFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  // We don't actually create a module for recirculation.
  return {};
}

} // namespace x86
} // namespace LibSynapse
