#pragma once

#include "controller_module.h"

namespace synapse {
namespace controller {

class CMSUpdate : public ControllerModule {
private:
  addr_t cms_addr;
  klee::ref<klee::Expr> key;

public:
  CMSUpdate(const Node *node, addr_t _cms_addr, klee::ref<klee::Expr> _key)
      : ControllerModule(ModuleType::Controller_CMSUpdate, "CMSUpdate", node),
        cms_addr(_cms_addr), key(_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new CMSUpdate(node, cms_addr, key);
    return cloned;
  }

  addr_t get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
};

class CMSUpdateFactory : public ControllerModuleFactory {
public:
  CMSUpdateFactory()
      : ControllerModuleFactory(ModuleType::Controller_CMSUpdate, "CMSUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace controller
} // namespace synapse