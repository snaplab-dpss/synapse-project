#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class ModifyHeader : public TofinoCPUModule {
private:
  addr_t chunk_addr;
  klee::ref<klee::Expr> original_chunk;
  std::vector<mod_t> changes;

public:
  ModifyHeader(const Node *node, addr_t _chunk_addr, const std::vector<mod_t> &_changes)
      : TofinoCPUModule(ModuleType::TofinoCPU_ModifyHeader, "ModifyHeader", node),
        chunk_addr(_chunk_addr), changes(_changes) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    ModifyHeader *cloned = new ModifyHeader(node, chunk_addr, changes);
    return cloned;
  }

  addr_t get_chunk_addr() const { return chunk_addr; }
  const std::vector<mod_t> &get_changes() const { return changes; }
};

class ModifyHeaderFactory : public TofinoCPUModuleFactory {
public:
  ModifyHeaderFactory()
      : TofinoCPUModuleFactory(ModuleType::TofinoCPU_ModifyHeader, "ModifyHeader") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino_cpu
