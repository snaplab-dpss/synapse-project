#include "send_to_controller.h"

namespace synapse {
namespace tofino {
namespace {
std::unique_ptr<BDD> replicate_hdr_parsing_ops(const EP *ep, const Node *node,
                                               const Node *&next) {
  std::vector<const Call *> prev_borrows =
      get_prev_functions(ep, node, {"packet_borrow_next_chunk"});
  std::vector<const Call *> prev_returns =
      get_prev_functions(ep, node, {"packet_return_chunk"});

  std::vector<const Node *> hdr_parsing_ops;
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_borrows.begin(), prev_borrows.end());
  hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_returns.begin(), prev_returns.end());

  if (hdr_parsing_ops.empty()) {
    return nullptr;
  }

  const BDD *old_bdd = ep->get_bdd();

  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);
  next = add_non_branch_nodes_to_bdd(new_bdd.get(), node, hdr_parsing_ops);

  return new_bdd;
}
} // namespace

std::optional<spec_impl_t> SendToControllerFactory::speculate(const EP *ep,
                                                              const Node *node,
                                                              const Context &ctx) const {
  Context new_ctx = ctx;

  hit_rate_t hr = new_ctx.get_profiler().get_hr(node);
  new_ctx.get_mutable_perf_oracle().add_controller_traffic(hr);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);
  spec_impl.next_target = TargetType::Controller;

  return spec_impl;
}

std::vector<impl_t> SendToControllerFactory::process_node(const EP *ep,
                                                          const Node *node) const {
  std::vector<impl_t> impls;

  // We can always send to the controller, at any point in time.
  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  symbols_t symbols = get_dataplane_state(ep, node);

  Module *module = new SendToController(node, symbols);
  EPNode *s2c_node = new EPNode(module);

  // Now we need to replicate the parsing operations that were done before.
  const Node *next = node;
  std::unique_ptr<BDD> new_bdd = replicate_hdr_parsing_ops(ep, node, next);

  // Note that we don't point to the next BDD node, as it was not actually
  // implemented.
  // We are delegating the implementation to other platform.
  EPLeaf leaf(s2c_node, next);
  new_ep->process_leaf(s2c_node, {leaf}, false);

  if (new_bdd) {
    new_ep->replace_bdd(std::move(new_bdd));
  }

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->parser_accept(ep, node);

  // TODO: How do we recalculate the estimated throughput after a forwarding
  // decision is made?
  ASSERT((!ep->get_active_leaf().node ||
          !forwarding_decision_already_made(ep->get_active_leaf().node)),
         "TODO");

  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_traffic(
      get_node_egress(new_ep, s2c_node));

  // FIXME: missing custom packet parsing for the SyNAPSE header.

  return impls;
}

} // namespace tofino
} // namespace synapse