#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

using Tofino::DS_ID;

class DataplaneFCFSCachedTableWrite : public ControllerModule {
private:
  DS_ID id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;

public:
  DataplaneFCFSCachedTableWrite(const BDDNode *_node, DS_ID _id, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys)
      : ControllerModule(ModuleType::Controller_DataplaneFCFSCachedTableWrite, "DataplaneFCFSCachedTableWrite", _node), id(_id), obj(_obj),
        keys(_keys) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    DataplaneFCFSCachedTableWrite *cloned = new DataplaneFCFSCachedTableWrite(node, id, obj, keys);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
};

class DataplaneFCFSCachedTableWriteFactory : public ControllerModuleFactory {
public:
  DataplaneFCFSCachedTableWriteFactory(const std::string &_instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_DataplaneFCFSCachedTableWrite, _instance_id), "DataplaneFCFSCachedTableWrite") {
  }

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
