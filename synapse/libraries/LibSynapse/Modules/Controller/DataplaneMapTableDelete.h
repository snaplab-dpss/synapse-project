#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneMapTableDelete : public ControllerModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;

public:
  DataplaneMapTableDelete(const LibBDD::Node *node, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys)
      : ControllerModule(ModuleType::Controller_DataplaneMapTableDelete, "DataplaneMapTableDelete", node), obj(_obj), keys(_keys) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    DataplaneMapTableDelete *cloned = new DataplaneMapTableDelete(node, obj, keys);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
};

class DataplaneMapTableDeleteFactory : public ControllerModuleFactory {
public:
  DataplaneMapTableDeleteFactory() : ControllerModuleFactory(ModuleType::Controller_DataplaneMapTableDelete, "DataplaneMapTableDelete") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse