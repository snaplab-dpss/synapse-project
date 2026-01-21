#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

using Tofino::DS_ID;

class DataplaneFCFSCachedSetRead : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key;
  symbol_t found;

public:
  DataplaneFCFSCachedSetRead(const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _key, symbol_t _found)
      : ControllerModule(ModuleType::Controller_DataplaneFCFSCachedSetRead, "DataplaneFCFSCachedSetRead", _node), obj(_obj), key(_key),
        found(_found) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneFCFSCachedSetRead(node, obj, key, found);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  const symbol_t &get_found() const { return found; }
};

class DataplaneFCFSCachedSetReadFactory : public ControllerModuleFactory {
public:
  DataplaneFCFSCachedSetReadFactory() : ControllerModuleFactory(ModuleType::Controller_DataplaneFCFSCachedSetRead, "DataplaneFCFSCachedSetRead") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse