#pragma once

#include "tofino_module.h"

namespace synapse {
namespace tofino {

class Drop : public TofinoModule {
public:
  Drop(const Node *node) : TofinoModule(ModuleType::Tofino_Drop, "Drop", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Drop *cloned = new Drop(node);
    return cloned;
  }
};

class DropFactory : public TofinoModuleFactory {
public:
  DropFactory() : TofinoModuleFactory(ModuleType::Tofino_Drop, "Drop") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino
} // namespace synapse