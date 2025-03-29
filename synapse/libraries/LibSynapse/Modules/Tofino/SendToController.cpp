#include <LibSynapse/Modules/Tofino/SendToController.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibBDD/Visitors/BDDVisualizer.h>

namespace LibSynapse {
namespace Tofino {

namespace {
std::unique_ptr<LibBDD::BDD> replicate_hdr_parsing_ops(const EP *ep, const LibBDD::Node *node, const LibBDD::Node *&next) {
  std::vector<const LibBDD::Call *> prev_borrows =
      node->get_prev_functions({"packet_borrow_next_chunk"}, ep->get_target_roots(ep->get_active_target()));
  std::vector<const LibBDD::Call *> prev_returns =
      node->get_prev_functions({"packet_return_chunk"}, ep->get_target_roots(ep->get_active_target()));

  std::vector<const LibBDD::Node *> hdr_parsing_ops;
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_borrows.begin(), prev_borrows.end());
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_returns.begin(), prev_returns.end());

  if (hdr_parsing_ops.empty()) {
    return nullptr;
  }

  const LibBDD::BDD *old_bdd = ep->get_bdd();

  std::unique_ptr<LibBDD::BDD> new_bdd = std::make_unique<LibBDD::BDD>(*old_bdd);
  next                                 = new_bdd->clone_and_add_non_branches(node, hdr_parsing_ops);

  return new_bdd;
}
} // namespace

std::optional<spec_impl_t> SendToControllerFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  Context new_ctx = ctx;

  const hit_rate_t hr = new_ctx.get_profiler().get_hr(node);
  new_ctx.get_mutable_perf_oracle().add_controller_traffic(hr);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);
  spec_impl.next_target = TargetType::Controller;

  return spec_impl;
}

std::vector<impl_t> SendToControllerFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                          LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  const EPLeaf active_leaf = ep->get_active_leaf();

  // We can't send to the controller if a forwarding decision was already made.
  if (active_leaf.node && active_leaf.node->forwarding_decision_already_made()) {
    return impls;
  }

  // Otherwise we can always send to the controller, at any point in time.
  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  const LibCore::Symbols symbols = get_relevant_dataplane_state(ep, node);

  Module *module   = new SendToController(node, symbols);
  EPNode *s2c_node = new EPNode(module);

  // Now we need to replicate the parsing operations that were done before.
  const LibBDD::Node *next             = node;
  std::unique_ptr<LibBDD::BDD> new_bdd = replicate_hdr_parsing_ops(ep, node, next);

  // Note that we don't point to the next BDD node, as it was not actually implemented.
  // We are delegating the implementation to other platform.
  EPLeaf leaf(s2c_node, next);
  new_ep->process_leaf(s2c_node, {leaf}, false);

  if (new_bdd) {
    new_ep->replace_bdd(std::move(new_bdd));
  }

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->parser_accept(ep, node);

  const hit_rate_t hr = new_ep->get_ctx().get_profiler().get_hr(s2c_node);
  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_traffic(new_ep->get_node_egress(hr, s2c_node));

  return impls;
}

std::unique_ptr<Module> SendToControllerFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  // We don't actually create a module for recirculation.
  return {};
}

} // namespace Tofino
} // namespace LibSynapse