#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class BloomFilterSet : public ControllerModule {
private:
  addr_t bf_addr;
  klee::ref<klee::Expr> key;

public:
  BloomFilterSet(const BDDNode *_node, addr_t _bf_addr, klee::ref<klee::Expr> _key)
      : ControllerModule(ModuleType::Controller_BloomFilterSet, "BloomFilterSet", _node), bf_addr(_bf_addr), key(_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new BloomFilterSet(node, bf_addr, key);
    return cloned;
  }

  addr_t get_bf_addr() const { return bf_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
};

class BloomFilterSetFactory : public ControllerModuleFactory {
public:
  BloomFilterSetFactory() : ControllerModuleFactory(ModuleType::Controller_BloomFilterSet, "BloomFilterSet") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse