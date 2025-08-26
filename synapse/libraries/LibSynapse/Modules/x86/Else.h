#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class Else : public x86Module {
public:
  Else(ModuleType _type, const BDDNode *_node) : x86Module(_type, "Else", _node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Else *cloned = new Else(type, node);
    return cloned;
  }
};

class ElseFactory : public x86ModuleFactory {
public:
  ElseFactory(const std::string &_instance_id) : x86ModuleFactory(ModuleType(ModuleCategory::x86_Else, _instance_id), "Else") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse