#pragma once

#include "tofino_cpu_module.h"

namespace synapse {
namespace tofino_cpu {

class CMSIncrement : public TofinoCPUModule {
private:
  addr_t cms_addr;
  klee::ref<klee::Expr> key;

public:
  CMSIncrement(const Node *node, addr_t _cms_addr, klee::ref<klee::Expr> _key)
      : TofinoCPUModule(ModuleType::TofinoCPU_CMSIncrement, "CMSIncrement", node),
        cms_addr(_cms_addr), key(_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new CMSIncrement(node, cms_addr, key);
    return cloned;
  }

  addr_t get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
};

class CMSIncrementFactory : public TofinoCPUModuleFactory {
public:
  CMSIncrementFactory()
      : TofinoCPUModuleFactory(ModuleType::TofinoCPU_CMSIncrement, "CMSIncrement") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino_cpu
} // namespace synapse