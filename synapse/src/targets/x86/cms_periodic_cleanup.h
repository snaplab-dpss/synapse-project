#pragma once

#include "x86_module.h"

namespace synapse {
namespace x86 {

class CMSPeriodicCleanup : public x86Module {
private:
  addr_t cms_addr;

public:
  CMSPeriodicCleanup(const Node *node, addr_t _cms_addr)
      : x86Module(ModuleType::x86_CMSPeriodicCleanup, "CMSPeriodicCleanup", node), cms_addr(_cms_addr) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new CMSPeriodicCleanup(node, cms_addr);
    return cloned;
  }

  addr_t get_cms_addr() const { return cms_addr; }
};

class CMSPeriodicCleanupFactory : public x86ModuleFactory {
public:
  CMSPeriodicCleanupFactory() : x86ModuleFactory(ModuleType::x86_CMSPeriodicCleanup, "CMSPeriodicCleanup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace x86
} // namespace synapse