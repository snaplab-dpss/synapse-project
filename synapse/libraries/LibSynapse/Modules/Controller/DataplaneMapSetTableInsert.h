#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneMapSetTableInsert : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key;
  symbol_t index_allocation_success;

public:
  DataplaneMapSetTableInsert(const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _key, const symbol_t &_index_allocation_success)
      : ControllerModule(ModuleType::Controller_DataplaneMapSetTableInsert, "DataplaneMapSetTableInsert", _node), obj(_obj), key(_key),
        index_allocation_success(_index_allocation_success) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    DataplaneMapSetTableInsert *cloned = new DataplaneMapSetTableInsert(node, obj, key, index_allocation_success);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  const symbol_t &get_index_allocation_success() const { return index_allocation_success; }
};

class DataplaneMapSetTableInsertFactory : public ControllerModuleFactory {
public:
  DataplaneMapSetTableInsertFactory() : ControllerModuleFactory(ModuleType::Controller_DataplaneMapSetTableInsert, "DataplaneMapSetTableInsert") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse