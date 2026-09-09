#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

// rotate_left(x, n) with a constant n: reorders the bits of x into metadata. Whole bytes move
// through the PHV (an ALU op); any other amount goes through the hash unit (@in_hash), as the
// SmartCookie expert does. It runs inside a ComputeAction (action_id), its own or one shared
// with independent steps of the same stage. An x that isn't a plain value (e.g. `a ^ b`) is
// computed first as an op of its own (`operands`, one entry).
class RotateLeft : public TofinoModule {
public:
  using compute_operand_t = TofinoModuleFactory::compute_operand_t;

private:
  DS_ID action_id;
  klee::ref<klee::Expr> x;
  u32 amount;
  klee::ref<klee::Expr> out;
  std::vector<compute_operand_t> operands;

public:
  RotateLeft(const BDDNode *_node, DS_ID _action_id, klee::ref<klee::Expr> _x, u32 _amount, klee::ref<klee::Expr> _out,
             const std::vector<compute_operand_t> &_operands = {})
      : TofinoModule(ModuleType::Tofino_RotateLeft, "RotateLeft", _node), action_id(_action_id), x(_x), amount(_amount), out(_out),
        operands(_operands) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new RotateLeft(node, action_id, x, amount, out, operands); }

  DS_ID get_action_id() const { return action_id; }
  std::string get_op_id() const;
  klee::ref<klee::Expr> get_x() const { return x; }
  u32 get_amount() const { return amount; }
  klee::ref<klee::Expr> get_out() const { return out; }
  // Every concat rotate goes through the hash unit, not just the non-byte-aligned ones. A
  // byte-aligned concat rotate is legal bare, but it still cuts its operand's container, and the
  // cut propagates along the dataflow (a = b ^ c forces b and c into compatible layouts) until a
  // 32-bit value needs one PHV source per piece, against a limit of two. Measured on SmartCookie:
  // wrapping the byte-aligned ones too took it from 116 unallocated slices to 84.
  bool uses_hash_unit() const { return amount != 0; }
  // Deliberately NOT mirrored in the op's ComputeOpKind, which stays Hash only for the
  // non-byte-aligned amounts. Charging every rotate against the per-action 32-bit hash budget and
  // the per-stage hash-distribution units makes the placer give up on the chain: measured, the
  // winner fell from 286.83 Mpps to 1.11 Kpps as the search offloaded it to the controller.
  // bf-p4c fits the wrapped program in 17 ingress / 18 egress stages, so the model, not the
  // hardware, is what refuses it. Emission and accounting are split until the model is fixed.
  const std::vector<compute_operand_t> &get_operands() const { return operands; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    std::unordered_set<DS_ID> ids{action_id};
    for (const compute_operand_t &operand : operands) {
      ids.insert(operand.action_id);
    }
    return ids;
  }
};

class RotateLeftFactory : public TofinoModuleFactory {
public:
  RotateLeftFactory() : TofinoModuleFactory(ModuleType::Tofino_RotateLeft, "RotateLeft") {}

  // Places the rotation `node` computes (operand first) through `builder`: the action holding
  // it, empty when `node` is not such a rotation or it doesn't fit.
  static std::optional<DS_ID> place(ComputeStepBuilder &builder, const EP *ep, const BDDNode *node, const speculations_t *speculations);

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
