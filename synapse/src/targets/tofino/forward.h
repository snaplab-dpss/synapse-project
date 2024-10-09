#pragma once

#include "tofino_module.h"

namespace tofino {

class Forward : public TofinoModule {
private:
  int dst_device;

public:
  Forward(const Node *node, int _dst_device)
      : TofinoModule(ModuleType::Tofino_Forward, "Forward", node),
        dst_device(_dst_device) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Forward *cloned = new Forward(node, dst_device);
    return cloned;
  }

  int get_dst_device() const { return dst_device; }

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

    // Look for other forwarding modules sending the packet to the same port.
    // If there are any, they share the same port capacity.
    // If they send more traffic than the port capacity, proportionality of
    // profiling hit rate should be considered.
    std::vector<const EPNode *> fwd_nodes = ep->get_nodes_by_type({
        ModuleType::Tofino_Forward,
    });

    hit_rate_t total_hr = hr;

    for (const EPNode *fwd_node : fwd_nodes) {
      const Module *fwd_module = fwd_node->get_module();
      assert(fwd_module->get_type() == ModuleType::Tofino_Forward);

      const Forward *fwd = dynamic_cast<const Forward *>(fwd_module);

      if (fwd->get_dst_device() != dst_device) {
        continue;
      }

      constraints_t fwd_constraints = ctx.get_node_constraints(fwd_node);
      std::optional<hit_rate_t> fwd_hr = profiler.get_fraction(fwd_constraints);
      assert(fwd_hr.has_value());

      total_hr += fwd_hr.value();
    }

    pps_t egress = ingress;

    // If the sum of all the traffic is more than the port capacity, then we
    // should keep proportionality of the profiling hit rate per forwarding
    // module.
    if (ingress > (hr / total_hr) * port_capacity) {
      egress = (hr / total_hr) * port_capacity;
    }

    return egress;
  }
};

class ForwardGenerator : public TofinoModuleGenerator {
public:
  ForwardGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_Forward, "Forward") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::ROUTE) {
      return std::nullopt;
    }

    const Route *route_node = static_cast<const Route *>(node);
    RouteOperation op = route_node->get_operation();

    if (op != RouteOperation::FWD) {
      return std::nullopt;
    }

    return spec_impl_t(decide(ep, node), ctx);
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::ROUTE) {
      return impls;
    }

    const Route *route_node = static_cast<const Route *>(node);
    RouteOperation op = route_node->get_operation();

    if (op != RouteOperation::FWD) {
      return impls;
    }

    int dst_device = route_node->get_dst_device();

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    Module *module = new Forward(node, dst_device);
    EPNode *ep_node = new EPNode(module);

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
    tofino_ctx->parser_accept(ep, node);

    return impls;
  }
};

} // namespace tofino
