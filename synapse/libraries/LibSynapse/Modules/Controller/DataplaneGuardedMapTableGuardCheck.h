#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneGuardedMapTableGuardCheck : public ControllerModule {
private:
  addr_t obj;
  symbol_t guard_allow;
  klee::ref<klee::Expr> guard_allow_condition;

public:
  DataplaneGuardedMapTableGuardCheck(const BDDNode *_node, addr_t _obj, const symbol_t &_guard_allow, klee::ref<klee::Expr> _guard_allow_condition)
      : ControllerModule(ModuleType::Controller_DataplaneGuardedMapTableGuardCheck, "DataplaneGuardedMapTableGuardCheck", _node), obj(_obj),
        guard_allow(_guard_allow), guard_allow_condition(_guard_allow_condition) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneGuardedMapTableGuardCheck(node, obj, guard_allow, guard_allow_condition);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const symbol_t &get_guard_allow() const { return guard_allow; }
  klee::ref<klee::Expr> get_guard_allow_condition() const { return guard_allow_condition; }
};

class DataplaneGuardedMapTableGuardCheckFactory : public ControllerModuleFactory {
public:
  DataplaneGuardedMapTableGuardCheckFactory()
      : ControllerModuleFactory(ModuleType::Controller_DataplaneGuardedMapTableGuardCheck, "DataplaneGuardedMapTableGuardCheck") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse