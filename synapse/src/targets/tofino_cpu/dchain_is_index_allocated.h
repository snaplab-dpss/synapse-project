#pragma once

#include "tofino_cpu_module.h"

namespace synapse {
namespace tofino_cpu {

class DchainIsIndexAllocated : public TofinoCPUModule {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> index;
  symbol_t is_allocated;

public:
  DchainIsIndexAllocated(const Node *node, addr_t _dchain_addr,
                         klee::ref<klee::Expr> _index, const symbol_t &_is_allocated)
      : TofinoCPUModule(ModuleType::TofinoCPU_DchainIsIndexAllocated,
                        "DchainIsIndexAllocated", node),
        dchain_addr(_dchain_addr), index(_index), is_allocated(_is_allocated) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new DchainIsIndexAllocated(node, dchain_addr, index, is_allocated);
    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  const symbol_t &get_is_allocated() const { return is_allocated; }
};

class DchainIsIndexAllocatedFactory : public TofinoCPUModuleFactory {
public:
  DchainIsIndexAllocatedFactory()
      : TofinoCPUModuleFactory(ModuleType::TofinoCPU_DchainIsIndexAllocated,
                               "DchainIsIndexAllocated") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino_cpu
} // namespace synapse