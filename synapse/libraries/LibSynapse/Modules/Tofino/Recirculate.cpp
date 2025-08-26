#include <LibSynapse/Modules/Tofino/Recirculate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

std::optional<spec_impl_t> RecirculateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  // No reason to speculatively predict recirculations.
  return {};
}

std::vector<impl_t> RecirculateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  const TofinoContext *tofino_ctx = get_tofino_ctx(ep);
  std::unordered_set<DS_ID> deps  = tofino_ctx->get_stateful_deps(ep, node);

  const EPLeaf active_leaf = ep->get_active_leaf();

  if (!active_leaf.node) {
    // Why would we recirculate if no decision was made yet?
    return {};
  }

  if (active_leaf.node->forwarding_decision_already_made()) {
    // We can't recirculate if a forwarding decision was already made.
    return {};
  }

  if (active_leaf.node->count_past_recirculations() > 4) {
    return {};
  }

  if (active_leaf.node->get_module()->get_type() == ModuleType::Tofino_Recirculate) {
    // Don't recirculate twice in a row.
    return {};
  }

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const hit_rate_t hr = new_ep->get_ctx().get_profiler().get_hr(ep->get_active_leaf().node);
  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_recirculated_traffic(ep->get_node_egress(hr, ep->get_active_leaf().node));

  const Symbols symbols = get_relevant_dataplane_state(ep, node);
  const u32 code_path   = node->get_id();

  Module *module  = new Recirculate(node, symbols, code_path);
  EPNode *ep_node = new EPNode(module);

  // Note that we don't point to the next BDD node, as it was not actually implemented.
  const EPLeaf leaf(ep_node, node);
  new_ep->process_leaf(ep_node, {leaf}, false);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));

  return impls;
}

std::unique_ptr<Module> RecirculateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  // We don't actually create a module for recirculation.
  return {};
}

} // namespace Tofino
} // namespace LibSynapse
