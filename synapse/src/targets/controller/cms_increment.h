#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

class CMSIncrement : public ControllerModule {
private:
  addr_t cms_addr;
  klee::ref<klee::Expr> key;

public:
  CMSIncrement(const Node *node, addr_t _cms_addr, klee::ref<klee::Expr> _key)
      : ControllerModule(ModuleType::Controller_CMSIncrement, "CMSIncrement", node), cms_addr(_cms_addr), key(_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new CMSIncrement(node, cms_addr, key);
    return cloned;
  }

  addr_t get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
};

class CMSIncrementFactory : public ControllerModuleFactory {
public:
  CMSIncrementFactory() : ControllerModuleFactory(ModuleType::Controller_CMSIncrement, "CMSIncrement") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse