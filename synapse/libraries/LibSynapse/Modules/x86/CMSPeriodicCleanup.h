#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class CMSPeriodicCleanup : public x86Module {
private:
  addr_t cms_addr;

public:
  CMSPeriodicCleanup(ModuleType _type, const BDDNode *_node, addr_t _cms_addr) : x86Module(_type, "CMSPeriodicCleanup", _node), cms_addr(_cms_addr) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new CMSPeriodicCleanup(type, node, cms_addr);
    return cloned;
  }

  addr_t get_cms_addr() const { return cms_addr; }
};

class CMSPeriodicCleanupFactory : public x86ModuleFactory {
public:
  CMSPeriodicCleanupFactory(const std::string &_instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_CMSPeriodicCleanup, _instance_id), "CMSPeriodicCleanup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse