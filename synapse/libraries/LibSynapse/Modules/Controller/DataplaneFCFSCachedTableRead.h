#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

using Tofino::DS_ID;

class DataplaneFCFSCachedTableRead : public ControllerModule {
private:
  DS_ID id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  std::optional<symbol_t> found;

public:
  DataplaneFCFSCachedTableRead(const std::string &_instance_id, const BDDNode *_node, DS_ID _id, addr_t _obj,
                               const std::vector<klee::ref<klee::Expr>> &_keys, klee::ref<klee::Expr> _value, std::optional<symbol_t> _found)
      : ControllerModule(ModuleType(ModuleCategory::Controller_DataplaneFCFSCachedTableRead, _instance_id), "DataplaneFCFSCachedTableRead", _node),
        id(_id), obj(_obj), keys(_keys), value(_value), found(_found) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneFCFSCachedTableRead(get_type().instance_id, node, id, obj, keys, value, found);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
  std::optional<symbol_t> get_found() const { return found; }
};

class DataplaneFCFSCachedTableReadFactory : public ControllerModuleFactory {
public:
  DataplaneFCFSCachedTableReadFactory(const std::string &_instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_DataplaneFCFSCachedTableRead, _instance_id), "DataplaneFCFSCachedTableRead") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse