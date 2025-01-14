#pragma once

#include "x86_module.hpp"

namespace synapse {
namespace x86 {

class ModifyHeader : public x86Module {
private:
  addr_t chunk_addr;
  std::vector<expr_mod_t> changes;

public:
  ModifyHeader(const Node *node, addr_t _chunk_addr, const std::vector<expr_mod_t> &_changes)
      : x86Module(ModuleType::x86_ModifyHeader, "ModifyHeader", node), chunk_addr(_chunk_addr), changes(_changes) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    ModifyHeader *cloned = new ModifyHeader(node, chunk_addr, changes);
    return cloned;
  }

  addr_t get_chunk_addr() const { return chunk_addr; }
  const std::vector<expr_mod_t> &get_changes() const { return changes; }
};

class ModifyHeaderFactory : public x86ModuleFactory {
public:
  ModifyHeaderFactory() : x86ModuleFactory(ModuleType::x86_ModifyHeader, "ModifyHeader") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace x86
} // namespace synapse