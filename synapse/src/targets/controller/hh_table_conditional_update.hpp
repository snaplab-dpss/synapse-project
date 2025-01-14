#pragma once

#include "controller_module.hpp"

namespace synapse {
namespace ctrl {

class HHTableConditionalUpdate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> write_value;
  symbol_t cache_write_failed;

public:
  HHTableConditionalUpdate(const Node *node, addr_t _obj, klee::ref<klee::Expr> _key,
                           klee::ref<klee::Expr> _write_value, const symbol_t &_cache_write_failed)
      : ControllerModule(ModuleType::Controller_HHTableConditionalUpdate, "HHTableConditionalUpdate", node), obj(_obj),
        key(_key), write_value(_write_value), cache_write_failed(_cache_write_failed) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new HHTableConditionalUpdate(node, obj, key, write_value, cache_write_failed);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }
  const symbol_t &get_cache_write_failed() const { return cache_write_failed; }
};

class HHTableConditionalUpdateFactory : public ControllerModuleFactory {
public:
  HHTableConditionalUpdateFactory()
      : ControllerModuleFactory(ModuleType::Controller_HHTableConditionalUpdate, "HHTableConditionalUpdate") {}

protected:
  std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse