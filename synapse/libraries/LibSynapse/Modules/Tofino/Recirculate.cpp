#include <LibSynapse/Modules/Tofino/Recirculate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

constexpr const char *RECIRCULATION_PORT_PARAM = "recirc_port";

namespace {
std::unique_ptr<EP> generate_new_ep(const EP *ep, const BDDNode *node, const Symbols &symbols, u16 recirc_port,
                                    const std::vector<u16> &past_recirculations) {
  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const hit_rate_t hr = new_ep->get_ctx().get_profiler().get_hr(ep->get_active_leaf().node);
  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_recirculated_traffic(recirc_port, ep->get_node_egress(hr, ep->get_active_leaf().node));

  const u32 code_path = node->get_id();

  Module *module  = new Recirculate(node, symbols, recirc_port, code_path);
  EPNode *ep_node = new EPNode(module);

  // Note that we don't point to the next BDD node, as it was not actually implemented.
  const EPLeaf leaf(ep_node, node);
  new_ep->process_leaf(ep_node, {leaf}, false);

  return new_ep;
}

std::unique_ptr<EP> concretize_single_port_recirc(const EP *ep, const BDDNode *node, const std::vector<u16> &past_recirc, u16 rport,
                                                  const Symbols &symbols) {
  bool marked           = false;
  bool returning_recirc = false;

  for (u16 past_rport : past_recirc) {
    if (marked && past_rport != rport) {
      returning_recirc = true;
      break;
    }

    if (past_rport == rport) {
      marked = true;
    }
  }

  // We don't support returning back to a past recirculation port after being sent to a different one.
  if (returning_recirc) {
    return nullptr;
  }

  return generate_new_ep(ep, node, symbols, rport, past_recirc);
}
} // namespace

std::optional<spec_impl_t> RecirculateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  // No reason to speculatively predict recirculations.
  return {};
}

std::vector<impl_t> RecirculateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  const TofinoContext *tofino_ctx = get_tofino_ctx(ep);
  std::unordered_set<DS_ID> deps  = tofino_ctx->get_stateful_deps(ep, node);

  // We can only recirculate if a stateful operation was implemented since the last recirculation.
  if (deps.empty()) {
    return {};
  }

  const EPLeaf active_leaf = ep->get_active_leaf();

  // We can't recirculate if a forwarding decision was already made.
  if (active_leaf.node && active_leaf.node->forwarding_decision_already_made()) {
    return {};
  }

  std::vector<u16> available_recirculation_ports;
  for (const tofino_recirculation_port_t &port : tofino_ctx->get_tna().tna_config.recirculation_ports) {
    available_recirculation_ports.push_back(port.dev_port);
  }

  const std::vector<u16> past_recirc = active_leaf.node->get_past_recirculations();
  const Symbols symbols              = get_relevant_dataplane_state(ep, node);

  std::vector<impl_t> impls;
  for (u16 rport : available_recirculation_ports) {
    std::unique_ptr<EP> new_ep = concretize_single_port_recirc(ep, node, past_recirc, rport, symbols);
    if (new_ep) {
      impl_t impl = implement(ep, node, std::move(new_ep), {{RECIRCULATION_PORT_PARAM, rport}});
      impls.push_back(std::move(impl));
    }
  }

  return impls;
}

std::unique_ptr<Module> RecirculateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  // We don't actually create a module for recirculation.
  return {};
}

} // namespace Tofino
} // namespace LibSynapse