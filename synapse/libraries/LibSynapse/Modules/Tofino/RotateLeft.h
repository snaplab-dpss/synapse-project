#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

// rotate_left(x, n) with a constant n: reorders the bits of x into metadata. Whole bytes move
// through the PHV (an ALU op); any other amount goes through the hash unit (@in_hash), as the
// SmartCookie expert does. It runs inside a ComputeAction (action_id), its own or one shared
// with independent steps of the same stage.
class RotateLeft : public TofinoModule {
private:
  DS_ID action_id;
  klee::ref<klee::Expr> x;
  u32 amount;
  klee::ref<klee::Expr> out;

public:
  RotateLeft(const BDDNode *_node, DS_ID _action_id, klee::ref<klee::Expr> _x, u32 _amount, klee::ref<klee::Expr> _out)
      : TofinoModule(ModuleType::Tofino_RotateLeft, "RotateLeft", _node), action_id(_action_id), x(_x), amount(_amount), out(_out) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new RotateLeft(node, action_id, x, amount, out); }

  DS_ID get_action_id() const { return action_id; }
  std::string get_op_id() const;
  klee::ref<klee::Expr> get_x() const { return x; }
  u32 get_amount() const { return amount; }
  klee::ref<klee::Expr> get_out() const { return out; }
  bool uses_hash_unit() const { return amount % 8 != 0; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {action_id}; }
};

class RotateLeftFactory : public TofinoModuleFactory {
public:
  RotateLeftFactory() : TofinoModuleFactory(ModuleType::Tofino_RotateLeft, "RotateLeft") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
