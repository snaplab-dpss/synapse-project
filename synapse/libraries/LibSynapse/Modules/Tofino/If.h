#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class If : public TofinoModule {
public:
  enum class ConditionActionHelper {
    None,
    CheckSignBitForLessThan32b,
    CheckSignBitForLessThanOrEqual32b,
    CheckSignBitForGreaterThan32b,
    CheckSignBitForGreaterThanOrEqual32b,
  };

  struct phv_limitation_workaround_t {
    If::ConditionActionHelper action_helper;
    klee::ref<klee::Expr> lhs;
    klee::ref<klee::Expr> rhs;

    phv_limitation_workaround_t() : action_helper(If::ConditionActionHelper::None), lhs(nullptr), rhs(nullptr) {}
    phv_limitation_workaround_t(If::ConditionActionHelper _action_helper, klee::ref<klee::Expr> _lhs, klee::ref<klee::Expr> _rhs)
        : action_helper(_action_helper), lhs(_lhs), rhs(_rhs) {}
  };

  struct condition_t {
    klee::ref<klee::Expr> expr;
    phv_limitation_workaround_t phv_limitation_workaround;

    condition_t(klee::ref<klee::Expr> _expr) : expr(_expr), phv_limitation_workaround() {}
    condition_t(klee::ref<klee::Expr> _expr, phv_limitation_workaround_t _phv_limitation_workaround)
        : expr(_expr), phv_limitation_workaround(_phv_limitation_workaround) {}
  };

private:
  klee::ref<klee::Expr> original_condition;

  // AND-changed conditions.
  // Every condition must be met to be equivalent to the original condition.
  // This will later be split into multiple branch conditions, each evaluating
  // each sub condition.
  std::vector<condition_t> conditions;

public:
  If(const BDDNode *_node, klee::ref<klee::Expr> _original_condition, const std::vector<condition_t> &_conditions)
      : TofinoModule(ModuleType::Tofino_If, "If", _node), original_condition(_original_condition), conditions(_conditions) {}

  If(const BDDNode *_node, klee::ref<klee::Expr> _original_condition)
      : TofinoModule(ModuleType::Tofino_If, "If", _node), original_condition(_original_condition), conditions({_original_condition}) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    If *cloned = new If(node, original_condition, conditions);
    return cloned;
  }

  klee::ref<klee::Expr> get_original_condition() const { return original_condition; }
  const std::vector<condition_t> &get_conditions() const { return conditions; }
};

class IfFactory : public TofinoModuleFactory {
public:
  IfFactory() : TofinoModuleFactory(ModuleType::Tofino_If, "If") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse