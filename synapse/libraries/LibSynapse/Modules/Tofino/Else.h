#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class Else : public TofinoModule {
public:
  Else(const BDDNode *_node) : TofinoModule(ModuleType::Tofino_Else, "Else", _node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Else *cloned = new Else(node);
    return cloned;
  }
};

class ElseFactory : public TofinoModuleFactory {
public:
  ElseFactory() : TofinoModuleFactory(ModuleType::Tofino_Else, "Else") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse