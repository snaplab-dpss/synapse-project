#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

// Controller-side (CPU) unrolled arithmetic operation (an op_* BDD node, see LibBDD/Unroll.h):
// computes `value` (a <op> b) and binds it for downstream use.
class ArithmeticOp : public ControllerModule {
private:
  klee::ref<klee::Expr> value;
  klee::ref<klee::Expr> out;

public:
  ArithmeticOp(const BDDNode *_node, klee::ref<klee::Expr> _value, klee::ref<klee::Expr> _out)
      : ControllerModule(ModuleType::Controller_ArithmeticOp, "ArithmeticOp", _node), value(_value), out(_out) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new ArithmeticOp(node, value, out); }

  klee::ref<klee::Expr> get_value() const { return value; }
  klee::ref<klee::Expr> get_out() const { return out; }
};

class ArithmeticOpFactory : public ControllerModuleFactory {
public:
  ArithmeticOpFactory() : ControllerModuleFactory(ModuleType::Controller_ArithmeticOp, "ArithmeticOp") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
