#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneGuardedMapTableDelete : public ControllerModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;

public:
  DataplaneGuardedMapTableDelete(const std::string &_instance_id, const BDDNode *_node, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys)
      : ControllerModule(ModuleType(ModuleCategory::Controller_DataplaneGuardedMapTableDelete, _instance_id), "DataplaneGuardedMapTableDelete",
                         _node),
        obj(_obj), keys(_keys) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    DataplaneGuardedMapTableDelete *cloned = new DataplaneGuardedMapTableDelete(get_type().instance_id, node, obj, keys);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
};

class DataplaneGuardedMapTableDeleteFactory : public ControllerModuleFactory {
public:
  DataplaneGuardedMapTableDeleteFactory(const std::string &_instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_DataplaneGuardedMapTableDelete, _instance_id),
                                "DataplaneGuardedMapTableDelete") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse