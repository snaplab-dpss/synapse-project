#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

using Tofino::DS_ID;

class DataplaneFCFSCachedTableRead : public ControllerModule {
private:
  DS_ID id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
  std::optional<symbol_t> found;

public:
  DataplaneFCFSCachedTableRead(const BDDNode *_node, DS_ID _id, addr_t _obj, klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _value,
                               std::optional<symbol_t> _found)
      : ControllerModule(ModuleType::Controller_DataplaneFCFSCachedTableRead, "DataplaneFCFSCachedTableRead", _node), id(_id), obj(_obj), key(_key),
        value(_value), found(_found) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneFCFSCachedTableRead(node, id, obj, key, value, found);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_value() const { return value; }
  std::optional<symbol_t> get_found() const { return found; }
};

class DataplaneFCFSCachedTableReadFactory : public ControllerModuleFactory {
public:
  DataplaneFCFSCachedTableReadFactory()
      : ControllerModuleFactory(ModuleType::Controller_DataplaneFCFSCachedTableRead, "DataplaneFCFSCachedTableRead") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse