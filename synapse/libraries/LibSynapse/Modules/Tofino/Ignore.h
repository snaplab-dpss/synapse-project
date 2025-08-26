#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class Ignore : public TofinoModule {
public:
  Ignore(ModuleType _type, const BDDNode *_node) : TofinoModule(_type, "Ignore", _node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    Ignore *cloned = new Ignore(type, node);
    return cloned;
  }
};

class IgnoreFactory : public TofinoModuleFactory {
public:
  IgnoreFactory(const std::string &_instance_id) : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_Ignore, _instance_id), "Ignore") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse