#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

using LibCore::expr_byte_swap_t;
using LibCore::expr_mod_t;

class ModifyHeader : public TofinoModule {
private:
  addr_t hdr_addr;
  klee::ref<klee::Expr> hdr;
  std::vector<expr_mod_t> changes;
  std::vector<expr_byte_swap_t> swaps;

public:
  ModifyHeader(const std::string &_instance_id, const BDDNode *_node, addr_t _hdr_addr, klee::ref<klee::Expr> _hdr,
               const std::vector<expr_mod_t> &_changes, const std::vector<expr_byte_swap_t> &_swaps)
      : TofinoModule(ModuleType(ModuleCategory::Tofino_ModifyHeader, _instance_id), "ModifyHeader", _node), hdr_addr(_hdr_addr), hdr(_hdr),
        changes(_changes), swaps(_swaps) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    ModifyHeader *cloned = new ModifyHeader(get_type().instance_id, node, hdr_addr, hdr, changes, swaps);
    return cloned;
  }

  addr_t get_hdr_addr() const { return hdr_addr; }
  klee::ref<klee::Expr> get_hdr() const { return hdr; }
  const std::vector<expr_mod_t> &get_changes() const { return changes; }
  const std::vector<expr_byte_swap_t> &get_swaps() const { return swaps; }
};

class ModifyHeaderFactory : public TofinoModuleFactory {
public:
  ModifyHeaderFactory(const std::string &_instance_id)
      : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_ModifyHeader, _instance_id), "ModifyHeader") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse