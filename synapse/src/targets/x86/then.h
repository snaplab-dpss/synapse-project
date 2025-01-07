#pragma once

#include "x86_module.h"

namespace synapse {
namespace x86 {

class Then : public x86Module {
public:
  Then(const Node *node) : x86Module(ModuleType::x86_Then, "Then", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Then *cloned = new Then(node);
    return cloned;
  }
};

class ThenFactory : public x86ModuleFactory {
public:
  ThenFactory() : x86ModuleFactory(ModuleType::x86_Then, "Then") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace x86
} // namespace synapse