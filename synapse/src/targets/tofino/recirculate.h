#pragma once

#include "tofino_module.h"

#define RECIRCULATION_PORT_PARAM "recirc_port"

namespace tofino {

class Recirculate : public TofinoModule {
private:
  symbols_t symbols;
  int recirc_port;

public:
  Recirculate(const Node *node, symbols_t _symbols, int _recirc_port)
      : TofinoModule(ModuleType::Tofino_Recirculate, "Recirculate", node),
        symbols(_symbols), recirc_port(_recirc_port) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Recirculate *cloned = new Recirculate(node, symbols, recirc_port);
    return cloned;
  }

  symbols_t get_symbols() const { return symbols; }
  int get_recirc_port() const { return recirc_port; }
};

class RecirculateGenerator : public TofinoModuleGenerator {
public:
  RecirculateGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_Recirculate, "Recirculate") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    // No reason to speculatively predict recirculations.
    return std::nullopt;
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    const TofinoContext *tofino_ctx = get_tofino_ctx(ep);
    std::unordered_set<DS_ID> deps = tofino_ctx->get_stateful_deps(ep, node);

    // We can only recirculate if a stateful operation was implemented since the
    // last recirculation.
    if (deps.empty()) {
      return impls;
    }

    EPLeaf active_leaf = ep->get_active_leaf();

    // TODO: How do we recalculate the estimated throughput after a forwarding
    // decision is made?
    ASSERT((!ep->get_active_leaf().node ||
            !forwarding_decision_already_made(ep->get_active_leaf().node)),
           "TODO");

    int total_recirc_ports =
        get_tofino_ctx(ep)->get_tna().get_properties().total_recirc_ports;
    std::vector<int> past_recirc = get_past_recirculations(active_leaf.node);

    symbols_t symbols = get_dataplane_state(ep, node);
    impls = concretize_single_port_recirc(ep, node, past_recirc,
                                          total_recirc_ports, symbols);

    return impls;
  }

private:
  std::vector<impl_t> concretize_single_port_recirc(
      const EP *ep, const Node *node, const std::vector<int> &past_recirc,
      int total_recirc_ports, const symbols_t &symbols) const {
    std::vector<impl_t> impls;

    for (int rport = 0; rport < total_recirc_ports; rport++) {
      bool marked = false;
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
        continue;
      }

      EP *new_ep = generate_new_ep(ep, node, symbols, rport, past_recirc);
      impls.push_back(
          implement(ep, node, new_ep, {{RECIRCULATION_PORT_PARAM, rport}}));
    }

    return impls;
  }

  EP *generate_new_ep(const EP *ep, const Node *node, const symbols_t &symbols,
                      int recirc_port,
                      const std::vector<int> &past_recirculations) const {
    EP *new_ep = new EP(*ep);

    Module *module = new Recirculate(node, symbols, recirc_port);
    EPNode *ep_node = new EPNode(module);

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
    hit_rate_t recirc_fraction = ep->get_active_leaf_hit_rate();

    int port_recirculations = 1;
    for (int p : past_recirculations) {
      if (p == recirc_port) {
        port_recirculations++;
      }
    }

    new_ep->get_mutable_ctx()
        .get_mutable_perf_oracle()
        .add_recirculated_traffic(
            recirc_port, get_node_egress(ep, ep->get_active_leaf().node));

    // Note that we don't point to the next BDD node, as it was not actually
    // implemented.
    EPLeaf leaf(ep_node, node);
    new_ep->process_leaf(ep_node, {leaf}, false);

    return new_ep;
  }
};

} // namespace tofino
