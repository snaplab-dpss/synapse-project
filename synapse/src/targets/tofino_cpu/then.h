#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class Then : public TofinoCPUModule {
public:
  Then(const Node *node)
      : TofinoCPUModule(ModuleType::TofinoCPU_Then, "Then", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Then *cloned = new Then(node);
    return cloned;
  }
};

class ThenGenerator : public TofinoCPUModuleGenerator {
public:
  ThenGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_Then, "Then") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override;
};

} // namespace tofino_cpu
