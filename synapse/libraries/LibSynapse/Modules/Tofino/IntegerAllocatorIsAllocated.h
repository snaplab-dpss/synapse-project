#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class IntegerAllocatorIsAllocated : public TofinoModule {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> index;
  symbol_t is_allocated;

public:
  IntegerAllocatorIsAllocated(const InstanceId _instance_id, const BDDNode *_node, addr_t _dchain_addr, klee::ref<klee::Expr> _index,
                              const symbol_t &_is_allocated)
      : TofinoModule(ModuleType(ModuleCategory::Tofino_IntegerAllocatorIsAllocated, _instance_id), "IntegerAllocatorIsAllocated", _node),
        dchain_addr(_dchain_addr), index(_index), is_allocated(_is_allocated) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new IntegerAllocatorIsAllocated(get_type().instance_id, node, dchain_addr, index, is_allocated);
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
  IntegerAllocatorIsAllocatedFactory(const InstanceId _instance_id)
      : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_IntegerAllocatorIsAllocated, _instance_id), "IntegerAllocatorIsAllocated") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse