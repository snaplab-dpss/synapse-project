#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class DchainFreeIndex : public TofinoCPUModule {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> index;

public:
  DchainFreeIndex(const Node *node, addr_t _dchain_addr,
                  klee::ref<klee::Expr> _index)
      : TofinoCPUModule(ModuleType::TofinoCPU_DchainFreeIndex,
                        "DchainFreeIndex", node),
        dchain_addr(_dchain_addr), index(_index) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new DchainFreeIndex(node, dchain_addr, index);
    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
};

class DchainFreeIndexGenerator : public TofinoCPUModuleGenerator {
public:
  DchainFreeIndexGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_DchainFreeIndex,
                                 "DchainFreeIndex") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override;
};

} // namespace tofino_cpu
