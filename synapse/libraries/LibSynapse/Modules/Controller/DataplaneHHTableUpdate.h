#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneHHTableUpdate : public ControllerModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;

public:
  DataplaneHHTableUpdate(const InstanceId _instance_id, const BDDNode *_node, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys,
                         klee::ref<klee::Expr> _value)
      : ControllerModule(ModuleType(ModuleCategory::Controller_DataplaneHHTableUpdate, _instance_id), "DataplaneHHTableUpdate", _node), obj(_obj),
        keys(_keys), value(_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneHHTableUpdate(get_type().instance_id, node, obj, keys, value);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  std::vector<klee::ref<klee::Expr>> get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
};

class DataplaneHHTableUpdateFactory : public ControllerModuleFactory {
public:
  DataplaneHHTableUpdateFactory(const InstanceId _instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_DataplaneHHTableUpdate, _instance_id), "DataplaneHHTableUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse