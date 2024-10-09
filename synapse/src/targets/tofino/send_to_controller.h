#pragma once

#include "tofino_module.h"

namespace tofino {

class SendToController : public TofinoModule {
private:
  symbols_t symbols;

public:
  SendToController(const Node *node, symbols_t _symbols)
      : TofinoModule(ModuleType::Tofino_SendToController, TargetType::TofinoCPU,
                     "SendToController", node),
        symbols(_symbols) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    SendToController *cloned = new SendToController(node, symbols);
    return cloned;
  }

  const symbols_t &get_symbols() const { return symbols; }

  virtual pps_t compute_egress_tput(const EP *ep,
                                    pps_t ingress) const override {
    const Context &ctx = ep->get_ctx();
    const Profiler &profiler = ctx.get_profiler();

    const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
    const TNA &tna = tofino_ctx->get_tna();
    const PerfOracle &perf_oracle = tna.get_perf_oracle();

    pps_t port_capacity = perf_oracle.get_port_capacity_pps();
    constraints_t constraints = node->get_ordered_branch_constraints();
    std::optional<hit_rate_t> opt_hr = profiler.get_fraction(constraints);
    assert(opt_hr.has_value());
    hit_rate_t hr = opt_hr.value();

    // Look for other send to controller modules, as they share the same
    // interface. If they send more traffic than the controller capacity,
    // proportionality of profiling hit rate should be considered.
    std::vector<const EPNode *> s2c_nodes = ep->get_nodes_by_type({
        ModuleType::Tofino_SendToController,
    });

    hit_rate_t total_hr = hr;

    for (const EPNode *s2c_node : s2c_nodes) {
      const Module *s2c_module = s2c_node->get_module();
      assert(s2c_module->get_type() == ModuleType::Tofino_SendToController);
      const SendToController *s2c =
          dynamic_cast<const SendToController *>(s2c_module);

      constraints_t s2c_constraints = ctx.get_node_constraints(s2c_node);
      std::optional<hit_rate_t> s2c_hr = profiler.get_fraction(s2c_constraints);
      assert(s2c_hr.has_value());

      total_hr += s2c_hr.value();
    }

    pps_t egress = ingress;

    // If the sum of all the traffic is more than the capacity, then we
    // should keep proportionality of the profiling hit rate per module.
    if (ingress > (hr / total_hr) * CONTROLLER_CAPACITY_PPS) {
      egress = (hr / total_hr) * CONTROLLER_CAPACITY_PPS;
    }

    return egress;
  }
};

class SendToControllerGenerator : public TofinoModuleGenerator {
public:
  SendToControllerGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_SendToController,
                              "SendToController") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    Context new_ctx = ctx;

    Profiler &profiler = new_ctx.get_mutable_profiler();
    constraints_t constraints = node->get_ordered_branch_constraints();

    std::optional<hit_rate_t> fraction = profiler.get_fraction(constraints);
    assert(fraction.has_value());

    new_ctx.update_traffic_fractions(TargetType::Tofino, TargetType::TofinoCPU,
                                     *fraction);

    spec_impl_t spec_impl(decide(ep, node), new_ctx);
    spec_impl.next_target = TargetType::TofinoCPU;

    return spec_impl;
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    // We can always send to the controller, at any point in time.
    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    symbols_t symbols = get_dataplane_state(ep, node);

    Module *module = new SendToController(node, symbols);
    EPNode *ep_node = new EPNode(module);

    // Now we need to replicate the parsing operations that were done before.
    BDD *new_bdd = nullptr;
    const Node *next = node;

    bool replicated_bdd = replicate_hdr_parsing_ops(ep, node, new_bdd, next);

    // Note that we don't point to the next BDD node, as it was not actually
    // implemented.
    // We are delegating the implementation to other platform.
    EPLeaf leaf(ep_node, next);
    new_ep->process_leaf(ep_node, {leaf}, false);

    if (replicated_bdd) {
      new_ep->replace_bdd(new_bdd);
    }

    // FIXME: missing custom packet parsing for the SyNAPSE header.

    return impls;
  }

private:
  bool replicate_hdr_parsing_ops(const EP *ep, const Node *node, BDD *&new_bdd,
                                 const Node *&next) const {
    std::vector<const Call *> prev_borrows =
        get_prev_functions(ep, node, {"packet_borrow_next_chunk"});
    std::vector<const Call *> prev_returns =
        get_prev_functions(ep, node, {"packet_return_chunk"});

    std::vector<const Node *> hdr_parsing_ops;
    hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_borrows.begin(),
                           prev_borrows.end());
    hdr_parsing_ops.insert(hdr_parsing_ops.end(), prev_returns.begin(),
                           prev_returns.end());

    if (hdr_parsing_ops.empty()) {
      return false;
    }

    const BDD *old_bdd = ep->get_bdd();

    new_bdd = new BDD(*old_bdd);
    Node *new_next;
    add_non_branch_nodes_to_bdd(ep, new_bdd, node, hdr_parsing_ops, new_next);

    next = new_next;

    return true;
  }
};

} // namespace tofino
