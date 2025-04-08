#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneMapTableLookup : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
  std::optional<LibCore::symbol_t> found;

public:
  DataplaneMapTableLookup(const LibBDD::Node *node, addr_t _obj, klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _value,
                          std::optional<LibCore::symbol_t> _found)
      : ControllerModule(ModuleType::Controller_DataplaneMapTableLookup, "DataplaneMapTableLookup", node), obj(_obj), key(_key), value(_value),
        found(_found) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneMapTableLookup(node, obj, key, value, found);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_value() const { return value; }
  std::optional<LibCore::symbol_t> get_found() const { return found; }
};

class DataplaneMapTableLookupFactory : public ControllerModuleFactory {
public:
  DataplaneMapTableLookupFactory() : ControllerModuleFactory(ModuleType::Controller_DataplaneMapTableLookup, "DataplaneMapTableLookup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse