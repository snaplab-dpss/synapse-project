#pragma once

#include "tofino_cpu_module.h"

#include "else.h"
#include "then.h"

namespace tofino_cpu {

class If : public TofinoCPUModule {
private:
  klee::ref<klee::Expr> condition;

public:
  If(const Node *node, klee::ref<klee::Expr> _condition)
      : TofinoCPUModule(ModuleType::TofinoCPU_If, "If", node),
        condition(_condition) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    If *cloned = new If(node, condition);
    return cloned;
  }

  klee::ref<klee::Expr> get_condition() const { return condition; }
};

class IfGenerator : public TofinoCPUModuleGenerator {
public:
  IfGenerator() : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_If, "If") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::Branch) {
      return std::nullopt;
    }

    return spec_impl_t(decide(ep, node), ctx);
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::Branch) {
      return impls;
    }

    const Branch *branch_node = static_cast<const Branch *>(node);

    klee::ref<klee::Expr> condition = branch_node->get_condition();

    assert(branch_node->get_on_true());
    assert(branch_node->get_on_false());

    Module *if_module = new If(node, condition);
    Module *then_module = new Then(node);
    Module *else_module = new Else(node);

    EPNode *if_node = new EPNode(if_module);
    EPNode *then_node = new EPNode(then_module);
    EPNode *else_node = new EPNode(else_module);

    if_node->set_constraint(condition);
    if_node->set_children(then_node, else_node);
    then_node->set_prev(if_node);
    else_node->set_prev(if_node);

    EPLeaf then_leaf(then_node, branch_node->get_on_true());
    EPLeaf else_leaf(else_node, branch_node->get_on_false());

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    new_ep->process_leaf(if_node, {then_leaf, else_leaf});

    return impls;
  }
};

} // namespace tofino_cpu
