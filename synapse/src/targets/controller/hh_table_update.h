#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

class HHTableUpdate : public ControllerModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  symbol_t min_estimate;

public:
  HHTableUpdate(const Node *node, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys,
                klee::ref<klee::Expr> _value, const symbol_t &_min_estimate)
      : ControllerModule(ModuleType::Controller_HHTableUpdate, "HHTableUpdate", node), obj(_obj),
        keys(_keys), value(_value), min_estimate(_min_estimate) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new HHTableUpdate(node, obj, keys, value, min_estimate);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  std::vector<klee::ref<klee::Expr>> get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
  const symbol_t &get_min_estimate() const { return min_estimate; }
};

class HHTableUpdateFactory : public ControllerModuleFactory {
public:
  HHTableUpdateFactory()
      : ControllerModuleFactory(ModuleType::Controller_HHTableUpdate, "HHTableUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse