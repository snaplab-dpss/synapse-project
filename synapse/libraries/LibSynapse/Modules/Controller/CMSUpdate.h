#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class CMSUpdate : public ControllerModule {
private:
  addr_t cms_addr;
  klee::ref<klee::Expr> key;

public:
  CMSUpdate(const std::string &_instance_id, const BDDNode *_node, addr_t _cms_addr, klee::ref<klee::Expr> _key)
      : ControllerModule(ModuleType(ModuleCategory::Controller_CMSUpdate, _instance_id), "CMSUpdate", _node), cms_addr(_cms_addr), key(_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new CMSUpdate(get_type().instance_id, node, cms_addr, key);
    return cloned;
  }

  addr_t get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
};

class CMSUpdateFactory : public ControllerModuleFactory {
public:
  CMSUpdateFactory(const std::string &_instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_CMSUpdate, _instance_id), "CMSUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse