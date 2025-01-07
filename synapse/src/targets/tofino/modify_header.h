#pragma once

#include "tofino_module.h"

namespace synapse {
namespace tofino {

class ModifyHeader : public TofinoModule {
private:
  addr_t hdr_addr;
  klee::ref<klee::Expr> hdr;
  std::vector<mod_t> changes;

public:
  ModifyHeader(const Node *node, addr_t _hdr_addr, klee::ref<klee::Expr> _hdr,
               const std::vector<mod_t> &_changes)
      : TofinoModule(ModuleType::Tofino_ModifyHeader, "ModifyHeader", node), hdr_addr(_hdr_addr),
        hdr(_hdr), changes(_changes) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    ModifyHeader *cloned = new ModifyHeader(node, hdr_addr, hdr, changes);
    return cloned;
  }

  addr_t get_hdr_addr() const { return hdr_addr; }
  klee::ref<klee::Expr> get_hdr() const { return hdr; }
  const std::vector<mod_t> &get_changes() const { return changes; }
};

class ModifyHeaderFactory : public TofinoModuleFactory {
public:
  ModifyHeaderFactory() : TofinoModuleFactory(ModuleType::Tofino_ModifyHeader, "ModifyHeader") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace tofino
} // namespace synapse