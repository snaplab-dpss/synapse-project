#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

// rotate_left(x, n) with a constant n that is not a whole number of bytes, done without the
// hash unit: (x << n) and (x >> (w-n)) in one stage, then their OR in the next. Two stages
// instead of RotateLeft's one, but no hash-distribution units, which are scarce (3 rotates
// per stage). Three ops in ComputeActions: the shifts (shl_action_id, shr_action_id, usually
// the same action) and the OR (or_action_id). An x that isn't a plain value is computed first
// as an op of its own (`operands`, one entry).
class RotateLeftShifts : public TofinoModule {
public:
  using compute_operand_t = TofinoModuleFactory::compute_operand_t;

private:
  DS_ID shl_action_id;
  DS_ID shr_action_id;
  DS_ID or_action_id;
  klee::ref<klee::Expr> x;
  u32 amount;
  klee::ref<klee::Expr> out;
  std::vector<compute_operand_t> operands;

public:
  RotateLeftShifts(const BDDNode *_node, DS_ID _shl_action_id, DS_ID _shr_action_id, DS_ID _or_action_id, klee::ref<klee::Expr> _x, u32 _amount,
                   klee::ref<klee::Expr> _out, const std::vector<compute_operand_t> &_operands = {})
      : TofinoModule(ModuleType::Tofino_RotateLeftShifts, "RotateLeftShifts", _node), shl_action_id(_shl_action_id), shr_action_id(_shr_action_id),
        or_action_id(_or_action_id), x(_x), amount(_amount), out(_out), operands(_operands) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new RotateLeftShifts(node, shl_action_id, shr_action_id, or_action_id, x, amount, out, operands); }

  DS_ID get_shl_action_id() const { return shl_action_id; }
  DS_ID get_shr_action_id() const { return shr_action_id; }
  DS_ID get_or_action_id() const { return or_action_id; }
  std::string get_shl_op_id() const;
  std::string get_shr_op_id() const;
  std::string get_or_op_id() const;
  klee::ref<klee::Expr> get_x() const { return x; }
  u32 get_amount() const { return amount; }
  klee::ref<klee::Expr> get_out() const { return out; }
  const std::vector<compute_operand_t> &get_operands() const { return operands; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    std::unordered_set<DS_ID> ids{shl_action_id, shr_action_id, or_action_id};
    for (const compute_operand_t &operand : operands) {
      ids.insert(operand.action_id);
    }
    return ids;
  }
};

class RotateLeftShiftsFactory : public TofinoModuleFactory {
public:
  RotateLeftShiftsFactory() : TofinoModuleFactory(ModuleType::Tofino_RotateLeftShifts, "RotateLeftShifts") {}

  // Places the rotation `node` computes as shifts (operand first) through `builder`: the action
  // holding the OR, empty when `node` is not such a rotation or it doesn't fit.
  static std::optional<DS_ID> place(ComputeStepBuilder &builder, const EP *ep, const BDDNode *node, const speculations_t *speculations);

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
