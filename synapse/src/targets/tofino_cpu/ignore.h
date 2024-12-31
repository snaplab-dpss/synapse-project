#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class Ignore : public TofinoCPUModule {
public:
  Ignore(const Node *node)
      : TofinoCPUModule(ModuleType::TofinoCPU_Ignore, "Ignore", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Ignore *cloned = new Ignore(node);
    return cloned;
  }
};

class IgnoreGenerator : public TofinoCPUModuleGenerator {
public:
  IgnoreGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_Ignore, "Ignore") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override;
};

} // namespace tofino_cpu
