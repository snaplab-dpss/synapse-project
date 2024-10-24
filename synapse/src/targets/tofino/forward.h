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

    hit_rate_t hr = ctx.get_node_hr(node);
    hit_rate_t total_hr = perf_oracle.get_fwd_traffic(dst_device);

    pps_t port_capacity = perf_oracle.get_port_capacity_pps();
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

    int dst_device = route_node->get_dst_device();

    Context new_ctx = ctx;
    TofinoContext *tofino_ctx = new_ctx.get_mutable_target_ctx<TofinoContext>();
    tofino_ctx->get_mutable_tna().get_mutable_perf_oracle().add_fwd_traffic(
        dst_device, new_ctx.get_node_hr(node));

    return spec_impl_t(decide(ep, node), new_ctx);
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
    EPNode *fwd_node = new EPNode(module);

    EPLeaf leaf(fwd_node, node->get_next());
    new_ep->process_leaf(fwd_node, {leaf});

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
    tofino_ctx->parser_accept(ep, node);
    tofino_ctx->get_mutable_tna().get_mutable_perf_oracle().add_fwd_traffic(
        dst_device, new_ep->get_ctx().get_node_hr(fwd_node));

    return impls;
  }
};

} // namespace tofino
