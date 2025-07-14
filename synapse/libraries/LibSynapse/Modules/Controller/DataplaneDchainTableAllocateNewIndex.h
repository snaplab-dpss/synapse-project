#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneDchainTableAllocateNewIndex : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> allocated_index;
  klee::ref<klee::Expr> success;

public:
  DataplaneDchainTableAllocateNewIndex(const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _allocated_index, klee::ref<klee::Expr> _success)
      : ControllerModule(ModuleType::Controller_DataplaneDchainTableAllocateNewIndex, "DataplaneDchainTableAllocateNewIndex", _node), obj(_obj),
        allocated_index(_allocated_index), success(_success) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    DataplaneDchainTableAllocateNewIndex *cloned = new DataplaneDchainTableAllocateNewIndex(node, obj, allocated_index, success);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_allocated_index() const { return allocated_index; }
  klee::ref<klee::Expr> get_success() const { return success; }
};

class DataplaneDchainTableAllocateNewIndexFactory : public ControllerModuleFactory {
public:
  DataplaneDchainTableAllocateNewIndexFactory()
      : ControllerModuleFactory(ModuleType::Controller_DataplaneDchainTableAllocateNewIndex, "DataplaneDchainTableAllocateNewIndex") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse