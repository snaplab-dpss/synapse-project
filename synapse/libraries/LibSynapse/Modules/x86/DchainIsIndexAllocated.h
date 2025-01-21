#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class DchainIsIndexAllocated : public x86Module {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> index;
  LibCore::symbol_t is_allocated;

public:
  DchainIsIndexAllocated(const LibBDD::Node *node, addr_t _dchain_addr, klee::ref<klee::Expr> _index,
                         const LibCore::symbol_t &_is_allocated)
      : x86Module(ModuleType::x86_DchainIsIndexAllocated, "DchainIsIndexAllocated", node), dchain_addr(_dchain_addr), index(_index),
        is_allocated(_is_allocated) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new DchainIsIndexAllocated(node, dchain_addr, index, is_allocated);
    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  const LibCore::symbol_t &get_is_allocated() const { return is_allocated; }
};

class DchainIsIndexAllocatedFactory : public x86ModuleFactory {
public:
  DchainIsIndexAllocatedFactory() : x86ModuleFactory(ModuleType::x86_DchainIsIndexAllocated, "DchainIsIndexAllocated") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
};

} // namespace x86
} // namespace LibSynapse