#pragma once

#include "tofino_module.h"

namespace tofino {

class CMSIncrement : public TofinoModule {
private:
  DS_ID cms_id;
  addr_t cms_addr;
  klee::ref<klee::Expr> key;

public:
  CMSIncrement(const Node *node, DS_ID _cms_id, addr_t _cms_addr,
               klee::ref<klee::Expr> _key)
      : TofinoModule(ModuleType::Tofino_CMSIncrement, "CMSIncrement", node),
        cms_id(_cms_id), cms_addr(_cms_addr), key(_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    return new CMSIncrement(node, cms_id, cms_addr, key);
  }

  DS_ID get_cms_id() const { return cms_id; }
  addr_t get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {cms_id}; }
};

class CMSIncrementFactory : public TofinoModuleFactory {
public:
  CMSIncrementFactory()
      : TofinoModuleFactory(ModuleType::Tofino_CMSIncrement, "CMSIncrement") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino
