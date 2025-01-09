#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

class VectorRegisterUpdate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> index;
  addr_t value_addr;
  std::vector<mod_t> modifications;

public:
  VectorRegisterUpdate(const Node *node, addr_t _obj, klee::ref<klee::Expr> _index, addr_t _value_addr,
                       const std::vector<mod_t> &_modifications)
      : ControllerModule(ModuleType::Controller_VectorRegisterUpdate, "VectorRegisterUpdate", node), obj(_obj), index(_index),
        value_addr(_value_addr), modifications(_modifications) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new VectorRegisterUpdate(node, obj, index, value_addr, modifications);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  addr_t get_value_addr() const { return value_addr; }

  const std::vector<mod_t> &get_modifications() const { return modifications; }
};

class VectorRegisterUpdateFactory : public ControllerModuleFactory {
public:
  VectorRegisterUpdateFactory() : ControllerModuleFactory(ModuleType::Controller_VectorRegisterUpdate, "VectorRegisterUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node, SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse