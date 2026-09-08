#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

// One unrolled arithmetic operation (an op_* BDD node, see LibBDD/Unroll.h) computing `value`
// (a <op> b) into metadata. It runs inside a ComputeAction (action_id): its own when nothing
// else in the current run of stateless steps can take it, otherwise one it shares with
// independent steps placed in the same stage. Operands that aren't plain values are computed
// first as ops of their own (`operands`).
class ArithmeticOp : public TofinoModule {
public:
  using compute_operand_t = TofinoModuleFactory::compute_operand_t;

private:
  DS_ID action_id;
  klee::ref<klee::Expr> value;
  klee::ref<klee::Expr> out;
  std::vector<compute_operand_t> operands;

public:
  ArithmeticOp(const BDDNode *_node, DS_ID _action_id, klee::ref<klee::Expr> _value, klee::ref<klee::Expr> _out,
               const std::vector<compute_operand_t> &_operands = {})
      : TofinoModule(ModuleType::Tofino_ArithmeticOp, "ArithmeticOp", _node), action_id(_action_id), value(_value), out(_out), operands(_operands) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new ArithmeticOp(node, action_id, value, out, operands); }

  DS_ID get_action_id() const { return action_id; }
  std::string get_op_id() const;
  klee::ref<klee::Expr> get_value() const { return value; }
  klee::ref<klee::Expr> get_out() const { return out; }
  const std::vector<compute_operand_t> &get_operands() const { return operands; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    std::unordered_set<DS_ID> ids{action_id};
    for (const compute_operand_t &operand : operands) {
      ids.insert(operand.action_id);
    }
    return ids;
  }
};

class ArithmeticOpFactory : public TofinoModuleFactory {
public:
  ArithmeticOpFactory() : TofinoModuleFactory(ModuleType::Tofino_ArithmeticOp, "ArithmeticOp") {}

  // Places the op `node` computes (operands first) through `builder`: the action holding it,
  // empty when `node` is not such an op or it doesn't fit.
  static std::optional<DS_ID> place(ComputeStepBuilder &builder, const EP *ep, const BDDNode *node, const speculations_t *speculations);

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
