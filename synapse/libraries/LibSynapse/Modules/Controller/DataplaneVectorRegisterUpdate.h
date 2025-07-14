#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

using LibCore::expr_mod_t;

class DataplaneVectorRegisterUpdate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> index;
  addr_t value_addr;
  klee::ref<klee::Expr> old_value;
  klee::ref<klee::Expr> new_value;
  std::vector<expr_mod_t> modifications;

public:
  DataplaneVectorRegisterUpdate(const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _index, addr_t _value_addr, klee::ref<klee::Expr> _old_value,
                                klee::ref<klee::Expr> _new_value, const std::vector<expr_mod_t> &_modifications)
      : ControllerModule(ModuleType::Controller_DataplaneVectorRegisterUpdate, "DataplaneVectorRegisterUpdate", _node), obj(_obj), index(_index),
        value_addr(_value_addr), old_value(_old_value), new_value(_new_value), modifications(_modifications) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneVectorRegisterUpdate(node, obj, index, value_addr, old_value, new_value, modifications);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  addr_t get_value_addr() const { return value_addr; }
  klee::ref<klee::Expr> get_old_value() const { return old_value; }
  klee::ref<klee::Expr> get_new_value() const { return new_value; }
  const std::vector<expr_mod_t> &get_modifications() const { return modifications; }
};

class DataplaneVectorRegisterUpdateFactory : public ControllerModuleFactory {
public:
  DataplaneVectorRegisterUpdateFactory()
      : ControllerModuleFactory(ModuleType::Controller_DataplaneVectorRegisterUpdate, "DataplaneVectorRegisterUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse