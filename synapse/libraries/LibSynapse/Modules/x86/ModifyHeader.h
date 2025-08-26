#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace x86 {

using LibCore::expr_mod_t;

class ModifyHeader : public x86Module {
private:
  addr_t chunk_addr;
  std::vector<expr_mod_t> changes;

public:
  ModifyHeader(ModuleType _type, const BDDNode *_node, addr_t _chunk_addr, const std::vector<expr_mod_t> &_changes)
      : x86Module(_type, "ModifyHeader", _node), chunk_addr(_chunk_addr), changes(_changes) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    ModifyHeader *cloned = new ModifyHeader(type, node, chunk_addr, changes);
    return cloned;
  }

  addr_t get_chunk_addr() const { return chunk_addr; }
  const std::vector<expr_mod_t> &get_changes() const { return changes; }
};

class ModifyHeaderFactory : public x86ModuleFactory {
public:
  ModifyHeaderFactory(const std::string &_instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_ModifyHeader, _instance_id), "ModifyHeader") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse