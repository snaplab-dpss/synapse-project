#pragma once

#include "x86_module.h"

#include "../tofino/forward.h"
#include "../tofino_cpu/forward.h"

namespace x86 {

class Forward : public x86Module {
private:
  int dst_device;

public:
  Forward(const Node *node, int _dst_device)
      : x86Module(ModuleType::x86_Forward, "Forward", node),
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

class ForwardGenerator : public x86ModuleGenerator {
public:
  ForwardGenerator() : x86ModuleGenerator(ModuleType::x86_Forward, "Forward") {}

protected:
  bool bdd_node_match_pattern(const Node *node) const {
    if (node->get_type() != NodeType::ROUTE) {
      return false;
    }

    const Route *route_node = static_cast<const Route *>(node);
    RouteOperation op = route_node->get_operation();

    if (op != RouteOperation::FWD) {
      return false;
    }

    return true;
  }

  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (!bdd_node_match_pattern(node)) {
      return std::nullopt;
    }

    const Route *route_node = static_cast<const Route *>(node);
    int dst_device = route_node->get_dst_device();

    Context new_ctx = ctx;
    update_fwd_tput_calcs(new_ctx, ep, route_node, dst_device);

    return spec_impl_t(decide(ep, node), new_ctx);
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (!bdd_node_match_pattern(node)) {
      return impls;
    }

    const Route *route_node = static_cast<const Route *>(node);
    int dst_device = route_node->get_dst_device();

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    Context &new_ctx = new_ep->get_mutable_ctx();
    update_fwd_tput_calcs(new_ctx, ep, route_node, dst_device);

    Module *module = new Forward(node, dst_device);
    EPNode *ep_node = new EPNode(module);

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }
};

} // namespace x86
