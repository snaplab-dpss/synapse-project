#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneDchainTableFreeIndex : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> index;

public:
  DataplaneDchainTableFreeIndex(ModuleType _type, const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _index)
      : ControllerModule(_type, "DataplaneDchainTableFreeIndex", _node), obj(_obj), index(_index) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    DataplaneDchainTableFreeIndex *cloned = new DataplaneDchainTableFreeIndex(type, node, obj, index);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
};

class DataplaneDchainTableFreeIndexFactory : public ControllerModuleFactory {
public:
  DataplaneDchainTableFreeIndexFactory(const std::string &_instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_DataplaneDchainTableFreeIndex, _instance_id), "DataplaneDchainTableFreeIndex") {
  }

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse