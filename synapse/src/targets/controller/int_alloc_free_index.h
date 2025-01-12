#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

class IntegerAllocatorFreeIndex : public ControllerModule {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> index;

public:
  IntegerAllocatorFreeIndex(const Node *node, addr_t _dchain_addr, klee::ref<klee::Expr> _index)
      : ControllerModule(ModuleType::Controller_IntegerAllocatorFreeIndex, "IntegerAllocatorFreeIndex", node),
        dchain_addr(_dchain_addr), index(_index) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new IntegerAllocatorFreeIndex(node, dchain_addr, index);
    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
};

class IntegerAllocatorFreeIndexFactory : public ControllerModuleFactory {
public:
  IntegerAllocatorFreeIndexFactory()
      : ControllerModuleFactory(ModuleType::Controller_IntegerAllocatorFreeIndex, "IntegerAllocatorFreeIndex") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node, SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse