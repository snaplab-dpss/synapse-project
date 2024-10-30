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

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Recirculate *cloned = new Recirculate(node, symbols, recirc_port);
    return cloned;
  }

  symbols_t get_symbols() const { return symbols; }
  int get_recirc_port() const { return recirc_port; }

  virtual pps_t compute_egress_tput(const EP *ep,
                                    pps_t ingress) const override {
    const Context &ctx = ep->get_ctx();
    const Profiler &profiler = ctx.get_profiler();

    const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
    const TNA &tna = tofino_ctx->get_tna();
    const PerfOracle &perf_oracle = tna.get_perf_oracle();

    EPLeaf active_leaf = ep->get_active_leaf();
    std::vector<int> past_recirculations =
        get_past_recirculations(active_leaf.node);

    int recirculations = 1;
    for (int p : past_recirculations) {
      if (p == recirc_port) {
        recirculations++;
      }
    }

    hit_rate_t hr = profiler.get_hr(node);
    hit_rate_t total_hr =
        perf_oracle.get_recirculated_traffic(recirc_port, recirculations);

    assert(total_hr >= 0);
    assert(hr <= total_hr);

    pps_t egress_port_pps = perf_oracle.get_recirculated_traffic_pps(
        recirc_port, recirculations, ingress);

    return (hr / total_hr) * egress_port_pps;
  }
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

    int total_recirc_ports =
        get_tofino_ctx(ep)->get_tna().get_properties().total_recirc_ports;

    EPLeaf active_leaf = ep->get_active_leaf();
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

    for (int recirc_port = 0; recirc_port < total_recirc_ports; recirc_port++) {
      EP *new_ep = generate_new_ep(ep, node, symbols, recirc_port, past_recirc);
      impls.push_back(implement(ep, node, new_ep,
                                {{RECIRCULATION_PORT_PARAM, recirc_port}}));
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

    tofino_ctx->get_mutable_tna()
        .get_mutable_perf_oracle()
        .add_recirculated_traffic(recirc_port, port_recirculations,
                                  recirc_fraction);

    // Note that we don't point to the next BDD node, as it was not actually
    // implemented.
    EPLeaf leaf(ep_node, node);
    new_ep->process_leaf(ep_node, {leaf}, false);

    return new_ep;
  }
};

} // namespace tofino
