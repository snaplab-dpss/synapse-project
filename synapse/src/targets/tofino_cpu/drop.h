#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class Drop : public TofinoCPUModule {
public:
  Drop(const Node *node)
      : TofinoCPUModule(ModuleType::TofinoCPU_Drop, "Drop", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
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
  speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override;
};

} // namespace tofino_cpu
