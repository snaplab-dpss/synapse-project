#pragma once

#include "tofino_module.h"

namespace tofino {

class Broadcast : public TofinoModule {
public:
  Broadcast(const Node *node)
      : TofinoModule(ModuleType::Tofino_Broadcast, "Broadcast", node) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Broadcast *cloned = new Broadcast(node);
    return cloned;
  }
};

class BroadcastGenerator : public TofinoModuleGenerator {
public:
  BroadcastGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_Broadcast, "Broadcast") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::ROUTE) {
      return std::nullopt;
    }

    const Route *route_node = static_cast<const Route *>(node);
    RouteOperation op = route_node->get_operation();

    if (op != RouteOperation::BCAST) {
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

    if (op != RouteOperation::BCAST) {
      return impls;
    }

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
    tofino_ctx->parser_accept(ep, node);

    Module *module = new Broadcast(node);
    EPNode *ep_node = new EPNode(module);

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }
};

} // namespace tofino
