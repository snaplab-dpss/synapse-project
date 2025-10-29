#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class DchainIsIndexAllocated : public x86Module {
private:
  klee::ref<klee::Expr> dchain_addr;
  klee::ref<klee::Expr> dchain_addr;
  klee::ref<klee::Expr> index;
  symbol_t is_allocated;

public:
  DchainIsIndexAllocated(const InstanceId _instance_id, const BDDNode *_node, klee::ref<klee::Expr> _dchain_addr, klee::ref<klee::Expr> _index,
                         const symbol_t &_is_allocated)
      : x86Module(ModuleType(ModuleCategory::x86_DchainIsIndexAllocated, _instance_id), "DchainIsIndexAllocated", _node), dchain_addr(_dchain_addr),
        index(_index), is_allocated(_is_allocated) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DchainIsIndexAllocated(get_type().instance_id, node, dchain_addr, index, is_allocated);
    Module *cloned = new DchainIsIndexAllocated(get_type().instance_id, node, dchain_addr, index, is_allocated);
    return cloned;
  }

  klee::ref<klee::Expr> get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  const symbol_t &get_is_allocated() const { return is_allocated; }
};

class DchainIsIndexAllocatedFactory : public x86ModuleFactory {
public:
  DchainIsIndexAllocatedFactory(const InstanceId _instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_DchainIsIndexAllocated, _instance_id), "DchainIsIndexAllocated") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse