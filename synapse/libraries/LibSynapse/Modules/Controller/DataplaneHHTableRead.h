#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneHHTableRead : public ControllerModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  LibCore::symbol_t map_has_this_key;
  LibCore::symbol_t min_estimate;

public:
  DataplaneHHTableRead(const LibBDD::Node *node, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys, klee::ref<klee::Expr> _value,
                       const LibCore::symbol_t &_map_has_this_key, const LibCore::symbol_t &_min_estimate)
      : ControllerModule(ModuleType::Controller_DataplaneHHTableRead, "DataplaneHHTableRead", node), obj(_obj), keys(_keys), value(_value),
        map_has_this_key(_map_has_this_key), min_estimate(_min_estimate) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneHHTableRead(node, obj, keys, value, map_has_this_key, min_estimate);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
  const LibCore::symbol_t &get_hit() const { return map_has_this_key; }
  const LibCore::symbol_t &get_min_estimate() const { return min_estimate; }
};

class DataplaneHHTableReadFactory : public ControllerModuleFactory {
public:
  DataplaneHHTableReadFactory() : ControllerModuleFactory(ModuleType::Controller_DataplaneHHTableRead, "DataplaneHHTableRead") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse