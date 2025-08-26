#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

using LibCore::expr_byte_swap_t;
using LibCore::expr_mod_t;

class ModifyHeader : public ControllerModule {
private:
  addr_t chunk_addr;
  klee::ref<klee::Expr> original_chunk;
  std::vector<expr_mod_t> changes;
  std::vector<expr_byte_swap_t> swaps;

public:
  ModifyHeader(ModuleType _type, const BDDNode *_node, addr_t _chunk_addr, const std::vector<expr_mod_t> &_changes,
               const std::vector<expr_byte_swap_t> &_swaps)
      : ControllerModule(_type, "ModifyHeader", _node), chunk_addr(_chunk_addr), changes(_changes), swaps(_swaps) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    ModifyHeader *cloned = new ModifyHeader(type, node, chunk_addr, changes, swaps);
    return cloned;
  }

  addr_t get_chunk_addr() const { return chunk_addr; }
  const std::vector<expr_mod_t> &get_changes() const { return changes; }
  const std::vector<expr_byte_swap_t> &get_swaps() const { return swaps; }
};

class ModifyHeaderFactory : public ControllerModuleFactory {
public:
  ModifyHeaderFactory(const std::string &_instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_ModifyHeader, _instance_id), "ModifyHeader") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse