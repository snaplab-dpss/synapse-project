#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneMapSetTableDelete : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key;

public:
  DataplaneMapSetTableDelete(const BDDNode *_node, addr_t _obj, const klee::ref<klee::Expr> &_key)
      : ControllerModule(ModuleType::Controller_DataplaneMapSetTableDelete, "DataplaneMapSetTableDelete", _node), obj(_obj), key(_key) {}
  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    DataplaneMapSetTableDelete *cloned = new DataplaneMapSetTableDelete(node, obj, key);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const klee::ref<klee::Expr> &get_key() const { return key; }
};

class DataplaneMapSetTableDeleteFactory : public ControllerModuleFactory {
public:
  DataplaneMapSetTableDeleteFactory() : ControllerModuleFactory(ModuleType::Controller_DataplaneMapSetTableDelete, "DataplaneMapSetTableDelete") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse