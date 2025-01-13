#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

class VectorRegisterLookup : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> value;

public:
  VectorRegisterLookup(const Node *node, addr_t _obj, klee::ref<klee::Expr> _index, klee::ref<klee::Expr> _value)
      : ControllerModule(ModuleType::Controller_VectorRegisterLookup, "VectorRegisterLookup", node), obj(_obj),
        index(_index), value(_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new VectorRegisterLookup(node, obj, index, value);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_value() const { return value; }
};

class VectorRegisterLookupFactory : public ControllerModuleFactory {
public:
  VectorRegisterLookupFactory()
      : ControllerModuleFactory(ModuleType::Controller_VectorRegisterLookup, "VectorRegisterLookup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse