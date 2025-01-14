#pragma once

#include "tofino_module.hpp"

namespace synapse {
namespace tofino {

class Ignore : public TofinoModule {
public:
  Ignore(const Node *node) : TofinoModule(ModuleType::Tofino_Ignore, "Ignore", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Ignore *cloned = new Ignore(node);
    return cloned;
  }
};

class IgnoreFactory : public TofinoModuleFactory {
public:
  IgnoreFactory() : TofinoModuleFactory(ModuleType::Tofino_Ignore, "Ignore") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace tofino
} // namespace synapse