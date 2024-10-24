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

    pps_t port_capacity = perf_oracle.get_port_capacity_pps();
    constraints_t constraints = node->get_ordered_branch_constraints();
    std::optional<hit_rate_t> opt_hr = profiler.get_hr(constraints);
    assert(opt_hr.has_value());
    hit_rate_t hr = opt_hr.value();

    // One for each consecutive recirculation to the same port.
    // E.g.:
    //    Index 0 contains the fraction of traffic recirculated once.
    //    Index 1 contains the fraction of traffic recirculated twice.
    //    Etc.
    std::vector<hit_rate_t> recirculations;

    // Get all the other recirculation nodes.

    return ingress;
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

    int total_recirc_ports = get_total_recirc_ports(ep);

    std::vector<int> past_recirc =
        get_past_recirc_ports(ep, total_recirc_ports);

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

  int get_total_recirc_ports(const EP *ep) const {
    const TofinoContext *ctx = get_tofino_ctx(ep);
    const TNA &tna = ctx->get_tna();
    const TNAProperties &properties = tna.get_properties();
    return properties.total_recirc_ports;
  }

  EP *generate_new_ep(const EP *ep, const Node *node, const symbols_t &symbols,
                      int recirc_port,
                      const std::vector<int> &past_recirc) const {
    EP *new_ep = new EP(*ep);

    Module *module = new Recirculate(node, symbols, recirc_port);
    EPNode *ep_node = new EPNode(module);

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
    hit_rate_t recirc_fraction = ep->get_active_leaf_hit_rate();

    int port_recirculations = 1;
    std::optional<int> past_recirc_port;

    for (int p : past_recirc) {
      if (p == recirc_port) {
        port_recirculations += 1;
      }

      if (!past_recirc_port.has_value()) {
        past_recirc_port = p;
      }
    }

    tofino_ctx->add_recirculated_traffic(recirc_port, port_recirculations,
                                         recirc_fraction, past_recirc_port);

    // Note that we don't point to the next BDD node, as it was not actually
    // implemented.
    // We are delegating the implementation to other platform.
    EPLeaf leaf(ep_node, node);
    new_ep->process_leaf(ep_node, {leaf}, false);

    return new_ep;
  }

  std::vector<int> get_past_recirc_ports(const EP *ep,
                                         int total_recirc_ports) const {
    EPLeaf active_leaf = ep->get_active_leaf();
    const EPNode *node = active_leaf.node;

    std::vector<int> past_recirc;

    while ((node = node->get_prev())) {
      const Module *module = node->get_module();

      if (!module) {
        continue;
      }

      if (module->get_type() == ModuleType::Tofino_Recirculate) {
        const Recirculate *recirc_module =
            static_cast<const Recirculate *>(module);
        int recirc_port = recirc_module->get_recirc_port();
        assert(recirc_port <= total_recirc_ports);

        past_recirc.push_back(recirc_port);
      }
    }

    return past_recirc;
  }
};

} // namespace tofino
