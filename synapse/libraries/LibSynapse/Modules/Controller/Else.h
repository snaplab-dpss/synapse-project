#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class Else : public ControllerModule {
public:
  Else(const InstanceId _instance_id, const BDDNode *_node)
      : ControllerModule(ModuleType(ModuleCategory::Controller_Else, _instance_id), "Else", _node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Else *cloned = new Else(get_type().instance_id, node);
    return cloned;
  }
};

class ElseFactory : public ControllerModuleFactory {
public:
  ElseFactory(const InstanceId _instance_id) : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_Else, _instance_id), "Else") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
