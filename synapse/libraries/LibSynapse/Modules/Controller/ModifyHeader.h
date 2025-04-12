#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class ModifyHeader : public ControllerModule {
private:
  addr_t chunk_addr;
  klee::ref<klee::Expr> original_chunk;
  std::vector<LibCore::expr_mod_t> changes;
  std::vector<LibCore::expr_byte_swap_t> swaps;

public:
  ModifyHeader(const LibBDD::Node *_node, addr_t _chunk_addr, const std::vector<LibCore::expr_mod_t> &_changes,
               const std::vector<LibCore::expr_byte_swap_t> &_swaps)
      : ControllerModule(ModuleType::Controller_ModifyHeader, "ModifyHeader", _node), chunk_addr(_chunk_addr), changes(_changes), swaps(_swaps) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    ModifyHeader *cloned = new ModifyHeader(node, chunk_addr, changes, swaps);
    return cloned;
  }

  addr_t get_chunk_addr() const { return chunk_addr; }
  const std::vector<LibCore::expr_mod_t> &get_changes() const { return changes; }
  const std::vector<LibCore::expr_byte_swap_t> &get_swaps() const { return swaps; }
};

class ModifyHeaderFactory : public ControllerModuleFactory {
public:
  ModifyHeaderFactory() : ControllerModuleFactory(ModuleType::Controller_ModifyHeader, "ModifyHeader") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse