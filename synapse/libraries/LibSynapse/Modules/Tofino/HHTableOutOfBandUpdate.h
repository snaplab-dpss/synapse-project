#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class HHTableOutOfBandUpdate : public TofinoModule {
private:
  DS_ID hh_table_id;
  addr_t obj;

public:
  HHTableOutOfBandUpdate(const LibBDD::Node *_node, DS_ID _hh_table_id, addr_t _obj)
      : TofinoModule(ModuleType::Tofino_HHTableOutOfBandUpdate, "HHTableOutOfBandUpdate", _node), hh_table_id(_hh_table_id), obj(_obj) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new HHTableOutOfBandUpdate(node, hh_table_id, obj);
    return cloned;
  }

  DS_ID get_hh_table_id() const { return hh_table_id; }
  addr_t get_obj() const { return obj; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {hh_table_id}; }
};

class HHTableOutOfBandUpdateFactory : public TofinoModuleFactory {
public:
  HHTableOutOfBandUpdateFactory() : TofinoModuleFactory(ModuleType::Tofino_HHTableOutOfBandUpdate, "HHTableOutOfBandUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse