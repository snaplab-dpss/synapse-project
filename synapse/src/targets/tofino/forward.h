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
    RouteOp op = route_node->get_operation();

    if (op != RouteOp::FWD) {
      return std::nullopt;
    }

    int dst_device = route_node->get_dst_device();

    Context new_ctx = ctx;
    new_ctx.get_mutable_perf_oracle().add_fwd_traffic(
        dst_device, new_ctx.get_profiler().get_hr(node));

    return spec_impl_t(decide(ep, node), new_ctx);
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::ROUTE) {
      return impls;
    }

    const Route *route_node = static_cast<const Route *>(node);
    RouteOp op = route_node->get_operation();

    if (op != RouteOp::FWD) {
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

    new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_fwd_traffic(
        dst_device, get_node_egress(new_ep, fwd_node));

    return impls;
  }
};

} // namespace tofino
