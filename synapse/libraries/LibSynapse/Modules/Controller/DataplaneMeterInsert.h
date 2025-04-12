#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneMeterInsert : public ControllerModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> success;

public:
  DataplaneMeterInsert(const LibBDD::Node *_node, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys, klee::ref<klee::Expr> _success)
      : ControllerModule(ModuleType::Controller_DataplaneMeterInsert, "DataplaneMeterInsert", _node), obj(_obj), keys(_keys), success(_success) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    DataplaneMeterInsert *cloned = new DataplaneMeterInsert(node, obj, keys, success);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_success() const { return success; }
};

class DataplaneMeterInsertFactory : public ControllerModuleFactory {
public:
  DataplaneMeterInsertFactory() : ControllerModuleFactory(ModuleType::Controller_DataplaneMeterInsert, "DataplaneMeterInsert") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse