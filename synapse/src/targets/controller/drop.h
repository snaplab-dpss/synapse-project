#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

class Drop : public ControllerModule {
public:
  Drop(const Node *node) : ControllerModule(ModuleType::Controller_Drop, "Drop", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Drop *cloned = new Drop(node);
    return cloned;
  }
};

class DropFactory : public ControllerModuleFactory {
public:
  DropFactory() : ControllerModuleFactory(ModuleType::Controller_Drop, "Drop") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace ctrl
} // namespace synapse