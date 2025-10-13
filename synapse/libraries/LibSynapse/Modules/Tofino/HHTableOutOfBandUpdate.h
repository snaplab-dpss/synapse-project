#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class HHTableOutOfBandUpdate : public TofinoModule {
private:
  DS_ID hh_table_id;
  addr_t obj;

public:
  HHTableOutOfBandUpdate(const InstanceId _instance_id, const BDDNode *_node, DS_ID _hh_table_id, addr_t _obj)
      : TofinoModule(ModuleType(ModuleCategory::Tofino_HHTableOutOfBandUpdate, _instance_id), "HHTableOutOfBandUpdate", _node),
        hh_table_id(_hh_table_id), obj(_obj) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new HHTableOutOfBandUpdate(get_type().instance_id, node, hh_table_id, obj);
    return cloned;
  }

  DS_ID get_hh_table_id() const { return hh_table_id; }
  addr_t get_obj() const { return obj; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {hh_table_id}; }
};

class HHTableOutOfBandUpdateFactory : public TofinoModuleFactory {
public:
  HHTableOutOfBandUpdateFactory(const InstanceId _instance_id)
      : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_HHTableOutOfBandUpdate, _instance_id), "HHTableOutOfBandUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse