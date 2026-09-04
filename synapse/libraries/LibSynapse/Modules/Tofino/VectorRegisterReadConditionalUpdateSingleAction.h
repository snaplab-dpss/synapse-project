#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class VectorRegisterReadConditionalUpdateSingleAction : public TofinoModule {
private:
  DS_ID id;
  addr_t obj;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;
  klee::ref<klee::Expr> condition;
  // When true this is a max/min swap: the register action returns the displaced
  // value (the "shadow" = min(value, write)) and the redundant branch is collapsed.
  bool returns_shadow;
  // When returns_shadow is true, the symbol the downstream min() call produced for
  // the shadow; the register action's output is bound to it so consumers resolve.
  klee::ref<klee::Expr> shadow_symbol;

public:
  VectorRegisterReadConditionalUpdateSingleAction(const BDDNode *_node, DS_ID _id, addr_t _obj, klee::ref<klee::Expr> _index,
                                                  klee::ref<klee::Expr> _read_value, klee::ref<klee::Expr> _write_value,
                                                  klee::ref<klee::Expr> _condition, bool _returns_shadow = false,
                                                  klee::ref<klee::Expr> _shadow_symbol = nullptr)
      : TofinoModule(ModuleType::Tofino_VectorRegisterReadConditionalUpdateSingleAction, "VectorRegisterReadConditionalUpdateSingleAction", _node),
        id(_id), obj(_obj), index(_index), read_value(_read_value), write_value(_write_value), condition(_condition),
        returns_shadow(_returns_shadow), shadow_symbol(_shadow_symbol) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new VectorRegisterReadConditionalUpdateSingleAction(node, id, obj, index, read_value, write_value, condition, returns_shadow,
                                                                         shadow_symbol);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_read_value() const { return read_value; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }
  klee::ref<klee::Expr> get_condition() const { return condition; }
  bool get_returns_shadow() const { return returns_shadow; }
  klee::ref<klee::Expr> get_shadow_symbol() const { return shadow_symbol; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {id}; }
};

class VectorRegisterReadConditionalUpdateSingleActionFactory : public TofinoModuleFactory {
public:
  VectorRegisterReadConditionalUpdateSingleActionFactory()
      : TofinoModuleFactory(ModuleType::Tofino_VectorRegisterReadConditionalUpdateSingleAction, "VectorRegisterReadConditionalUpdateSingleAction") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse