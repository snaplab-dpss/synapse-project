#pragma once

#include "tofino_module.h"

namespace tofino {

class Drop : public TofinoModule {
public:
  Drop(const Node *node)
      : TofinoModule(ModuleType::Tofino_Drop, "Drop", node) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Drop *cloned = new Drop(node);
    return cloned;
  }
};

class DropGenerator : public TofinoModuleGenerator {
public:
  DropGenerator() : TofinoModuleGenerator(ModuleType::Tofino_Drop, "Drop") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::ROUTE) {
      return std::nullopt;
    }

    const Route *route_node = static_cast<const Route *>(node);
    RouteOp op = route_node->get_operation();

    if (op != RouteOp::DROP) {
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
    RouteOp op = route_node->get_operation();

    if (op != RouteOp::DROP) {
      return impls;
    }

    if (is_parser_reject(ep)) {
      return impls;
    }

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    Module *module = new Drop(node);
    EPNode *ep_node = new EPNode(module);

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
    tofino_ctx->parser_accept(ep, node);

    return impls;
  }

private:
  bool is_parser_reject(const EP *ep) const {
    EPLeaf leaf = ep->get_active_leaf();

    if (!leaf.node || !leaf.node->get_prev()) {
      return false;
    }

    const EPNode *node = leaf.node;
    const EPNode *prev = node->get_prev();
    const Module *module = prev->get_module();

    ModuleType type = module->get_type();
    return (type == ModuleType::Tofino_ParserCondition);
  }
};

} // namespace tofino
