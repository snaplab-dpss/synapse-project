#pragma once

#include "x86_module.h"

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

  virtual std::optional<speculation_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (bdd_node_match_pattern(node))
      return ctx;
    return std::nullopt;
  }

  virtual std::vector<__generator_product_t>
  process_node(const EP *ep, const Node *node) const override {
    std::vector<__generator_product_t> products;

    if (!bdd_node_match_pattern(node)) {
      return products;
    }

    const Route *route_node = static_cast<const Route *>(node);
    int dst_device = route_node->get_dst_device();

    Module *module = new Forward(node, dst_device);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    products.emplace_back(new_ep);

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return products;
  }
};

} // namespace x86
