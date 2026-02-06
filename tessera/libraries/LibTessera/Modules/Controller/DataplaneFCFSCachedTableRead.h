#pragma once

#include <LibTessera/Modules/Controller/ControllerModule.h>

namespace LibTessera {
namespace Controller {

using Tofino::DS_ID;

class DataplaneFCFSCachedTableRead : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
  symbol_t found;

public:
  DataplaneFCFSCachedTableRead(const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _value, const symbol_t &_found)
      : ControllerModule(ModuleType::Controller_DataplaneFCFSCachedTableRead, "DataplaneFCFSCachedTableRead", _node), obj(_obj), key(_key),
        value(_value), found(_found) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneFCFSCachedTableRead(node, obj, key, value, found);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_value() const { return value; }
  const symbol_t &get_found() const { return found; }
};

class DataplaneFCFSCachedTableReadFactory : public ControllerModuleFactory {
public:
  DataplaneFCFSCachedTableReadFactory()
      : ControllerModuleFactory(ModuleType::Controller_DataplaneFCFSCachedTableRead, "DataplaneFCFSCachedTableRead") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibTessera