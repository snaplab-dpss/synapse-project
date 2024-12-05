#pragma once

#include "x86_module.h"

namespace x86 {

class Ignore : public x86Module {
public:
  Ignore(const Node *node)
      : x86Module(ModuleType::x86_Ignore, "Ignore", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Ignore *cloned = new Ignore(node);
    return cloned;
  }
};

class IgnoreGenerator : public x86ModuleGenerator {
public:
  IgnoreGenerator() : x86ModuleGenerator(ModuleType::x86_Ignore, "Ignore") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    return std::nullopt;
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (!should_ignore(ep, node)) {
      return impls;
    }

    Module *module = new Ignore(node);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }

private:
  bool should_ignore(const EP *ep, const Node *node) const {
    if (node->get_type() != NodeType::CALL) {
      return false;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name == "vector_return") {
      return is_vector_return_without_modifications(ep, call_node);
    }

    return false;
  }
};

} // namespace x86
