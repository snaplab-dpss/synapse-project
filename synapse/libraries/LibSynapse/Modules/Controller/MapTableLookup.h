#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class MapTableLookup : public ControllerModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  std::optional<LibCore::symbol_t> found;

public:
  MapTableLookup(const LibBDD::Node *node, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys, klee::ref<klee::Expr> _value,
                 std::optional<LibCore::symbol_t> _found)
      : ControllerModule(ModuleType::Controller_MapTableLookup, "MapTableLookup", node), obj(_obj), keys(_keys), value(_value),
        found(_found) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new MapTableLookup(node, obj, keys, value, found);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
  std::optional<LibCore::symbol_t> get_found() const { return found; }
};

class MapTableLookupFactory : public ControllerModuleFactory {
public:
  MapTableLookupFactory() : ControllerModuleFactory(ModuleType::Controller_MapTableLookup, "MapTableLookup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse