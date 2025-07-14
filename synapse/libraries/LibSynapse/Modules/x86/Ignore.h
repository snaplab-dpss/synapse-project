#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class Ignore : public x86Module {
public:
  Ignore(const BDDNode *_node) : x86Module(ModuleType::x86_Ignore, "Ignore", _node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    Ignore *cloned = new Ignore(node);
    return cloned;
  }
};

class IgnoreFactory : public x86ModuleFactory {
public:
  IgnoreFactory() : x86ModuleFactory(ModuleType::x86_Ignore, "Ignore") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse