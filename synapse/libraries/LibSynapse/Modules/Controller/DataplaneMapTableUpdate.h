#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneMapTableUpdate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;

public:
  DataplaneMapTableUpdate(const InstanceId _instance_id, const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _value)
      : ControllerModule(ModuleType(ModuleCategory::Controller_DataplaneMapTableUpdate, _instance_id), "DataplaneMapTableUpdate", _node), obj(_obj),
        key(_key), value(_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    DataplaneMapTableUpdate *cloned = new DataplaneMapTableUpdate(get_type().instance_id, node, obj, key, value);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_value() const { return value; }
};

class DataplaneMapTableUpdateFactory : public ControllerModuleFactory {
public:
  DataplaneMapTableUpdateFactory(const InstanceId _instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_DataplaneMapTableUpdate, _instance_id), "DataplaneMapTableUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
