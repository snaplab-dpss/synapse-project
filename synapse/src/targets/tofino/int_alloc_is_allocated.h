#pragma once

#include "tofino_module.h"

namespace synapse {
namespace tofino {

class IntegerAllocatorIsAllocated : public TofinoModule {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> index;
  symbol_t is_allocated;

public:
  IntegerAllocatorIsAllocated(const Node *node, addr_t _dchain_addr, klee::ref<klee::Expr> _index,
                              const symbol_t &_is_allocated)
      : TofinoModule(ModuleType::Tofino_IntegerAllocatorIsAllocated, "IntegerAllocatorIsAllocated", node),
        dchain_addr(_dchain_addr), index(_index), is_allocated(_is_allocated) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new IntegerAllocatorIsAllocated(node, dchain_addr, index, is_allocated);
    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  const symbol_t &get_is_allocated() const { return is_allocated; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    // FIXME: Missing the Integer Allocator data structure.
    // return {table_id};
    return {};
  }
};

class IntegerAllocatorIsAllocatedFactory : public TofinoModuleFactory {
public:
  IntegerAllocatorIsAllocatedFactory()
      : TofinoModuleFactory(ModuleType::Tofino_IntegerAllocatorIsAllocated, "IntegerAllocatorIsAllocated") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace tofino
} // namespace synapse