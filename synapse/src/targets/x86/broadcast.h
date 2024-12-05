#pragma once

#include "x86_module.h"

namespace x86 {

class Broadcast : public x86Module {
public:
  Broadcast(const Node *node)
      : x86Module(ModuleType::x86_Broadcast, "Broadcast", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Broadcast *cloned = new Broadcast(node);
    return cloned;
  }
};

class BroadcastGenerator : public x86ModuleGenerator {
public:
  BroadcastGenerator()
      : x86ModuleGenerator(ModuleType::x86_Broadcast, "Broadcast") {}

protected:
  bool bdd_node_match_pattern(const Node *node) const {
    if (node->get_type() != NodeType::ROUTE) {
      return false;
    }

    const Route *route_node = static_cast<const Route *>(node);
    RouteOp op = route_node->get_operation();

    if (op != RouteOp::BCAST) {
      return false;
    }

    return true;
  }

  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (bdd_node_match_pattern(node))
      return spec_impl_t(decide(ep, node), ctx);
    return std::nullopt;
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (!bdd_node_match_pattern(node)) {
      return impls;
    }

    Module *module = new Broadcast(node);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }
};

} // namespace x86
