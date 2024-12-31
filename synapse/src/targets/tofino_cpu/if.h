#pragma once

#include "tofino_cpu_module.h"

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
  speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override;
};

} // namespace tofino_cpu
