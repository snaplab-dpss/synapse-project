#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class Drop : public TofinoCPUModule {
public:
  Drop(const Node *node)
      : TofinoCPUModule(ModuleType::TofinoCPU_Drop, "Drop", node) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Drop *cloned = new Drop(node);
    return cloned;
  }
};

class DropGenerator : public TofinoCPUModuleGenerator {
public:
  DropGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_Drop, "Drop") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::ROUTE) {
      return std::nullopt;
    }

    const Route *route_node = static_cast<const Route *>(node);
    RouteOperation op = route_node->get_operation();

    if (op != RouteOperation::DROP) {
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

    if (op != RouteOperation::DROP) {
      return impls;
    }

    Module *module = new Drop(node);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }
};

} // namespace tofino_cpu
