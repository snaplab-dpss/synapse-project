#pragma once

#include "tofino_cpu_module.h"

namespace synapse {
namespace tofino_cpu {

class Else : public TofinoCPUModule {
public:
  Else(const Node *node) : TofinoCPUModule(ModuleType::TofinoCPU_Else, "Else", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Else *cloned = new Else(node);
    return cloned;
  }
};

class ElseFactory : public TofinoCPUModuleFactory {
public:
  ElseFactory() : TofinoCPUModuleFactory(ModuleType::TofinoCPU_Else, "Else") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino_cpu
} // namespace synapse