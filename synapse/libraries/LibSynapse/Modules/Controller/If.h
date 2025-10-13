#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class If : public ControllerModule {
private:
  klee::ref<klee::Expr> condition;

public:
  If(const InstanceId _instance_id, const BDDNode *_node, klee::ref<klee::Expr> _condition)
      : ControllerModule(ModuleType(ModuleCategory::Controller_If, _instance_id), "If", _node), condition(_condition) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    If *cloned = new If(get_type().instance_id, node, condition);
    return cloned;
  }

  klee::ref<klee::Expr> get_condition() const { return condition; }
};

class IfFactory : public ControllerModuleFactory {
public:
  IfFactory(const InstanceId _instance_id) : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_If, _instance_id), "If") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse