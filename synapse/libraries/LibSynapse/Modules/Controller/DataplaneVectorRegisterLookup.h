#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneVectorRegisterLookup : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> value;

public:
  DataplaneVectorRegisterLookup(const std::string &_instance_id, const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _index,
                                klee::ref<klee::Expr> _value)
      : ControllerModule(ModuleType(ModuleCategory::Controller_DataplaneVectorRegisterLookup, _instance_id), "DataplaneVectorRegisterLookup", _node),
        obj(_obj), index(_index), value(_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneVectorRegisterLookup(get_type().instance_id, node, obj, index, value);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_value() const { return value; }
};

class DataplaneVectorRegisterLookupFactory : public ControllerModuleFactory {
public:
  DataplaneVectorRegisterLookupFactory(const std::string &_instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_DataplaneVectorRegisterLookup, _instance_id), "DataplaneVectorRegisterLookup") {
  }

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse