#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneGuardedMapTableUpdate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;

public:
  DataplaneGuardedMapTableUpdate(const LibBDD::Node *_node, addr_t _obj, klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _value)
      : ControllerModule(ModuleType::Controller_DataplaneGuardedMapTableUpdate, "DataplaneGuardedMapTableUpdate", _node), obj(_obj), key(_key),
        value(_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    DataplaneGuardedMapTableUpdate *cloned = new DataplaneGuardedMapTableUpdate(node, obj, key, value);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_value() const { return value; }
};

class DataplaneGuardedMapTableUpdateFactory : public ControllerModuleFactory {
public:
  DataplaneGuardedMapTableUpdateFactory()
      : ControllerModuleFactory(ModuleType::Controller_DataplaneGuardedMapTableUpdate, "DataplaneGuardedMapTableUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse