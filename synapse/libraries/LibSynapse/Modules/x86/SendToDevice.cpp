#include <LibSynapse/Modules/x86/SendToDevice.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibBDD/Visitors/BDDVisualizer.h>
#include <LibSynapse/Visualizers/EPVisualizer.h>
#include <cassert>
#include <klee/util/Ref.h>

#include <LibBDD/Visitors/BDDVisualizer.h>

namespace LibSynapse {
namespace x86 {
using LibBDD::Call;
using LibBDD::call_t;
namespace {
bool bdd_node_match_pattern(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    // std::cerr << "Node skipped: Not a Call node\n";
    return false;
  }

  const Call *call_op = dynamic_cast<const Call *>(node);
  const call_t call   = call_op->get_call();

  if (call.function_name != "send_to_device") {
    // std::cerr << "Node skipped: Function name is " << call.function_name << "\n";
    return false;
  }

  return true;
}

std::unique_ptr<BDD> replicate_hdr_parsing_ops(const EP *ep, const BDDNode *node, const BDDNode *&next) {
  std::vector<const Call *> prev_borrows = node->get_prev_functions({"packet_borrow_next_chunk"}, ep->get_target_roots(ep->get_active_target()));
  std::vector<const Call *> prev_returns = node->get_prev_functions({"packet_return_chunk"}, ep->get_target_roots(ep->get_active_target()));

  std::vector<const BDDNode *> hdr_parsing_ops;
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_borrows.begin(), prev_borrows.end());
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_returns.begin(), prev_returns.end());

  if (hdr_parsing_ops.empty()) {
    return nullptr;
  }

  const BDD *old_bdd = ep->get_bdd();

  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);
  new_bdd->add_cloned_non_branches(node->get_id(), hdr_parsing_ops);

  next = new_bdd->get_node_by_id(node->get_id());

  std::cerr << "NEXT ID: " << next->get_id() << "\n";

  BDDViz::visualize(new_bdd.get(), false);

  return new_bdd;
}
} // namespace

std::optional<spec_impl_t> SendToDeviceFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  Context new_ctx = ctx;

  // Don't send to the device if the node is already a route.
  if (node->get_type() == BDDNodeType::Route) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> next_target_expr = call.args.at("next_target").expr;

  bits_t width                       = next_target_expr->getWidth();
  const klee::ConstantExpr *constant = dynamic_cast<const klee::ConstantExpr *>(next_target_expr.get());

  assert(width <= 64 && "Width too big");
  InstanceId next_target_id = constant->getZExtValue(width);

  // We can always send to the device, at any point in time.
  spec_impl_t spec_impl(decide(ep, node), new_ctx);
  spec_impl.next_target = ep->get_target_by_id(next_target_id);

  return spec_impl;
}

std::vector<impl_t> SendToDeviceFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  const EPLeaf active_leaf = ep->get_active_leaf();
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  // We can't send to the device if a forwarding decision was already made.
  if (active_leaf.node && active_leaf.node->forwarding_decision_already_made()) {
    return {};
  }

  // Don't send to the device if the node is already a route.
  if (node->get_type() == BDDNodeType::Route) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> outgoing_port    = call.args.at("outgoing_port").expr;
  klee::ref<klee::Expr> next_target_expr = call.args.at("next_target").expr;

  bits_t width                       = next_target_expr->getWidth();
  const klee::ConstantExpr *constant = dynamic_cast<const klee::ConstantExpr *>(next_target_expr.get());

  assert(width <= 64 && "Width too big");
  InstanceId next_target_id = constant->getZExtValue(width);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);
  Symbols symbols            = get_relevant_dataplane_state(ep, node, get_target());
  symbols.remove("packet_chunks");

  Module *module   = new SendToDevice(get_type().instance_id, node, ep->get_target_by_id(next_target_id), outgoing_port, symbols);
  EPNode *s2d_node = new EPNode(module);

  EPNode *ep_node_leaf = s2d_node;

  const BDDNode *next          = node->get_next();
  std::unique_ptr<BDD> new_bdd = replicate_hdr_parsing_ops(ep, node, next);

  EPLeaf leaf(ep_node_leaf, next);
  new_ep->process_leaf(s2d_node, {leaf});

  if (new_bdd) {
    new_ep->replace_bdd(std::move(new_bdd));
  }

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
