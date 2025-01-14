#pragma once

#include "x86_module.hpp"

namespace synapse {
namespace x86 {

class Drop : public x86Module {
public:
  Drop(const Node *node) : x86Module(ModuleType::x86_Drop, "Drop", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Drop *cloned = new Drop(node);
    return cloned;
  }
};

class DropFactory : public x86ModuleFactory {
public:
  DropFactory() : x86ModuleFactory(ModuleType::x86_Drop, "Drop") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace x86
} // namespace synapse