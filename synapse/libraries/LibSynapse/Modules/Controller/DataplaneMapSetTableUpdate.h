#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneMapSetTableUpdate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key;

public:
  DataplaneMapSetTableUpdate(const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _key)
      : ControllerModule(ModuleType::Controller_DataplaneMapSetTableUpdate, "DataplaneMapSetTableUpdate", _node), obj(_obj), key(_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    DataplaneMapSetTableUpdate *cloned = new DataplaneMapSetTableUpdate(node, obj, key);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
};

class DataplaneMapSetTableUpdateFactory : public ControllerModuleFactory {
public:
  DataplaneMapSetTableUpdateFactory() : ControllerModuleFactory(ModuleType::Controller_DataplaneMapSetTableUpdate, "DataplaneMapSetTableUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse