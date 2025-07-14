#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DchainFreeIndex : public ControllerModule {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> index;

public:
  DchainFreeIndex(const BDDNode *_node, addr_t _dchain_addr, klee::ref<klee::Expr> _index)
      : ControllerModule(ModuleType::Controller_DchainFreeIndex, "DchainFreeIndex", _node), dchain_addr(_dchain_addr), index(_index) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DchainFreeIndex(node, dchain_addr, index);
    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
};

class DchainFreeIndexFactory : public ControllerModuleFactory {
public:
  DchainFreeIndexFactory() : ControllerModuleFactory(ModuleType::Controller_DchainFreeIndex, "DchainFreeIndex") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse