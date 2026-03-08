#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class CMSIncrement : public x86Module {
private:
  klee::ref<klee::Expr> cms_addr;
  klee::ref<klee::Expr> key_addr;
  klee::ref<klee::Expr> key;

public:
  CMSIncrement(const BDDNode *_node, klee::ref<klee::Expr> _cms_addr, klee::ref<klee::Expr> _key_addr, klee::ref<klee::Expr> _key)
      : x86Module(ModuleType::x86_CMSIncrement, "CMSIncrement", _node), cms_addr(_cms_addr), key_addr(_key_addr), key(_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new CMSIncrement(node, cms_addr, key_addr, key);
    return cloned;
  }

  klee::ref<klee::Expr> get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_key_addr() const { return key_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
};

class CMSIncrementFactory : public x86ModuleFactory {
public:
  CMSIncrementFactory() : x86ModuleFactory(ModuleType::x86_CMSIncrement, "CMSIncrement") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse