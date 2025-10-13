#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class Then : public ControllerModule {
public:
  Then(const InstanceId _instance_id, const BDDNode *_node)
      : ControllerModule(ModuleType(ModuleCategory::Controller_Then, _instance_id), "Then", _node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Then *cloned = new Then(get_type().instance_id, node);
    return cloned;
  }
};

class ThenFactory : public ControllerModuleFactory {
public:
  ThenFactory(const InstanceId _instance_id) : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_Then, _instance_id), "Then") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse