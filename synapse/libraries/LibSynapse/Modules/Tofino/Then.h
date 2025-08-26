#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class Then : public TofinoModule {
public:
  Then(ModuleType _type, const BDDNode *_node) : TofinoModule(_type, "Then", _node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Then *cloned = new Then(type, node);
    return cloned;
  }
};

class ThenFactory : public TofinoModuleFactory {
public:
  ThenFactory(const std::string &_instance_id) : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_Then, _instance_id), "Then") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse