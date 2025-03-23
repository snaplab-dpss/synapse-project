#include <LibSynapse/Modules/Tofino/Recirculate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

constexpr const char *RECIRCULATION_PORT_PARAM = "recirc_port";

namespace {
EP *generate_new_ep(const EP *ep, const LibBDD::Node *node, const LibCore::Symbols &symbols, int recirc_port,
                    const std::vector<int> &past_recirculations) {
  int port_recirculations = 1;
  for (int p : past_recirculations) {
    if (p == recirc_port) {
      port_recirculations++;
    }
  }

  u32 code_path = node->get_id();

  EP *new_ep = new EP(*ep);

  const hit_rate_t hr = new_ep->get_ctx().get_profiler().get_hr(ep->get_active_leaf().node);
  new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_recirculated_traffic(recirc_port,
                                                                               ep->get_node_egress(hr, ep->get_active_leaf().node));

  Module *module  = new Recirculate(node, symbols, recirc_port, code_path);
  EPNode *ep_node = new EPNode(module);

  // Note that we don't point to the next LibBDD::BDD node, as it was not actually
  // implemented.
  EPLeaf leaf(ep_node, node);
  new_ep->process_leaf(ep_node, {leaf}, false);

  return new_ep;
}

EP *concretize_single_port_recirc(const EP *ep, const LibBDD::Node *node, const std::vector<int> &past_recirc, int rport,
                                  const LibCore::Symbols &symbols) {
  bool marked           = false;
  bool returning_recirc = false;

  for (int past_rport : past_recirc) {
    if (marked && past_rport != rport) {
      returning_recirc = true;
      break;
    }

    if (past_rport == rport) {
      marked = true;
    }
  }

  // We don't support returning back to a past recirculation port after
  // being sent to a different one.
  if (returning_recirc) {
    return nullptr;
  }

  return generate_new_ep(ep, node, symbols, rport, past_recirc);
}
} // namespace

std::optional<spec_impl_t> RecirculateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  // No reason to speculatively predict recirculations.
  return std::nullopt;
}

std::vector<impl_t> RecirculateFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  const TofinoContext *tofino_ctx = get_tofino_ctx(ep);
  std::unordered_set<DS_ID> deps  = tofino_ctx->get_stateful_deps(ep, node);

  // We can only recirculate if a stateful operation was implemented since the
  // last recirculation.
  if (deps.empty()) {
    return impls;
  }

  EPLeaf active_leaf = ep->get_active_leaf();

  // TODO: How do we recalculate the estimated throughput after a forwarding
  // decision is made?
  assert((!ep->get_active_leaf().node || !ep->get_active_leaf().node->forwarding_decision_already_made()) && "TODO");

  int total_recirc_ports       = get_tofino_ctx(ep)->get_tna().get_properties().total_recirc_ports;
  std::vector<int> past_recirc = active_leaf.node->get_past_recirculations();

  LibCore::Symbols symbols = get_dataplane_state(ep, node);

  for (int rport = 0; rport < total_recirc_ports; rport++) {
    EP *new_ep = concretize_single_port_recirc(ep, node, past_recirc, rport, symbols);
    if (new_ep) {
      impl_t impl = implement(ep, node, new_ep, {{RECIRCULATION_PORT_PARAM, rport}});
      impls.push_back(impl);
    }
  }

  return impls;
}

std::unique_ptr<Module> RecirculateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  // We don't actually create a module for recirculation.
  return {};
}

} // namespace Tofino
} // namespace LibSynapse