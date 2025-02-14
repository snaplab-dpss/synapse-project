#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class ModifyHeader : public TofinoModule {
private:
  addr_t hdr_addr;
  klee::ref<klee::Expr> hdr;
  std::vector<LibCore::expr_mod_t> changes;
  std::vector<LibCore::expr_byte_swap_t> swaps;

public:
  ModifyHeader(const LibBDD::Node *node, addr_t _hdr_addr, klee::ref<klee::Expr> _hdr, const std::vector<LibCore::expr_mod_t> &_changes,
               const std::vector<LibCore::expr_byte_swap_t> &_swaps)
      : TofinoModule(ModuleType::Tofino_ModifyHeader, "ModifyHeader", node), hdr_addr(_hdr_addr), hdr(_hdr), changes(_changes),
        swaps(_swaps) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    ModifyHeader *cloned = new ModifyHeader(node, hdr_addr, hdr, changes, swaps);
    return cloned;
  }

  addr_t get_hdr_addr() const { return hdr_addr; }
  klee::ref<klee::Expr> get_hdr() const { return hdr; }
  const std::vector<LibCore::expr_mod_t> &get_changes() const { return changes; }
  const std::vector<LibCore::expr_byte_swap_t> &get_swaps() const { return swaps; }
};

class ModifyHeaderFactory : public TofinoModuleFactory {
public:
  ModifyHeaderFactory() : TofinoModuleFactory(ModuleType::Tofino_ModifyHeader, "ModifyHeader") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse