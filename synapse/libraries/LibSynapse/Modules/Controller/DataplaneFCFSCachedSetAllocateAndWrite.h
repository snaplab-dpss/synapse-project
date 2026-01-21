#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

using Tofino::DS_ID;

class DataplaneFCFSCachedSetAllocateAndWrite : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key;
  symbol_t allocation_successful;

public:
  DataplaneFCFSCachedSetAllocateAndWrite(const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _key, const symbol_t &_allocation_successful)
      : ControllerModule(ModuleType::Controller_DataplaneFCFSCachedSetAllocateAndWrite, "DataplaneFCFSCachedSetAllocateAndWrite", _node), obj(_obj),
        key(_key), allocation_successful(_allocation_successful) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    DataplaneFCFSCachedSetAllocateAndWrite *cloned = new DataplaneFCFSCachedSetAllocateAndWrite(node, obj, key, allocation_successful);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  const symbol_t &get_allocation_successful_symbol() const { return allocation_successful; }
};

class DataplaneFCFSCachedSetAllocateAndWriteFactory : public ControllerModuleFactory {
public:
  DataplaneFCFSCachedSetAllocateAndWriteFactory()
      : ControllerModuleFactory(ModuleType::Controller_DataplaneFCFSCachedSetAllocateAndWrite, "DataplaneFCFSCachedSetAllocateAndWrite") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse