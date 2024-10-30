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
    return perf_estimator(ep->get_ctx(), node, ingress);
  }

  static pps_t perf_estimator(const Context &ctx, const Node *node,
                              pps_t ingress) {
    const Profiler &profiler = ctx.get_profiler();

    const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
    const TNA &tna = tofino_ctx->get_tna();
    const PerfOracle &perf_oracle = tna.get_perf_oracle();

    hit_rate_t hr = ctx.get_profiler().get_hr(node);
    hit_rate_t total_hr = perf_oracle.get_controller_traffic();

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

    hit_rate_t fraction = profiler.get_hr(node);
    new_ctx.update_traffic_fractions(TargetType::Tofino, TargetType::TofinoCPU,
                                     fraction);

    TofinoContext *tofino_ctx = new_ctx.get_mutable_target_ctx<TofinoContext>();
    tofino_ctx->get_mutable_tna()
        .get_mutable_perf_oracle()
        .add_controller_traffic(fraction);

    spec_impl_t spec_impl(decide(ep, node), new_ctx,
                          SendToController::perf_estimator);
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
    EPNode *s2c_node = new EPNode(module);

    // Now we need to replicate the parsing operations that were done before.
    BDD *new_bdd = nullptr;
    const Node *next = node;

    bool replicated_bdd = replicate_hdr_parsing_ops(ep, node, new_bdd, next);

    // Note that we don't point to the next BDD node, as it was not actually
    // implemented.
    // We are delegating the implementation to other platform.
    EPLeaf leaf(s2c_node, next);
    new_ep->process_leaf(s2c_node, {leaf}, false);

    if (replicated_bdd) {
      new_ep->replace_bdd(new_bdd);
    }

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
    tofino_ctx->parser_accept(ep, node);
    tofino_ctx->get_mutable_tna()
        .get_mutable_perf_oracle()
        .add_controller_traffic(new_ep->get_ctx().get_profiler().get_hr(node));

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
