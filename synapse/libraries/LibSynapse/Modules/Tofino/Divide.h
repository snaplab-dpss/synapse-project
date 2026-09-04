#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

// Integer division with a compile-time numerator, lowered to a MathUnit
// (MathOp_t.DIV) driven by a dummy 1-entry register, exactly as the expert
// HyperLogLog P4 does it.
class Divide : public TofinoModule {
private:
  DS_ID reg_id;
  klee::ref<klee::Expr> denominator;
  u64 numerator;
  klee::ref<klee::Expr> quotient;

public:
  Divide(const BDDNode *_node, DS_ID _reg_id, klee::ref<klee::Expr> _denominator, u64 _numerator, klee::ref<klee::Expr> _quotient)
      : TofinoModule(ModuleType::Tofino_Divide, "Divide", _node), reg_id(_reg_id), denominator(_denominator), numerator(_numerator),
        quotient(_quotient) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new Divide(node, reg_id, denominator, numerator, quotient); }

  DS_ID get_reg_id() const { return reg_id; }
  klee::ref<klee::Expr> get_denominator() const { return denominator; }
  u64 get_numerator() const { return numerator; }
  klee::ref<klee::Expr> get_quotient() const { return quotient; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {reg_id}; }
};

class DivideFactory : public TofinoModuleFactory {
public:
  DivideFactory() : TofinoModuleFactory(ModuleType::Tofino_Divide, "Divide") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
