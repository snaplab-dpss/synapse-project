#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneHHTableOutOfBandUpdate : public ControllerModule {
private:
  addr_t obj;

public:
  DataplaneHHTableOutOfBandUpdate(const InstanceId _instance_id, const BDDNode *_node, addr_t _obj)
      : ControllerModule(ModuleType(ModuleCategory::Controller_DataplaneHHTableOutOfBandUpdate, _instance_id), "DataplaneHHTableOutOfBandUpdate",
                         _node),
        obj(_obj) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneHHTableOutOfBandUpdate(get_type().instance_id, node, obj);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
};

class DataplaneHHTableOutOfBandUpdateFactory : public ControllerModuleFactory {
public:
  DataplaneHHTableOutOfBandUpdateFactory(const InstanceId _instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Tofino_HHTableOutOfBandUpdate, _instance_id), "DataplaneHHTableOutOfBandUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
