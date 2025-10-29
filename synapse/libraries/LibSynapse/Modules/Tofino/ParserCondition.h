#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

class ParserCondition : public TofinoModule {
private:
  klee::ref<klee::Expr> condition;

public:
  ParserCondition(const InstanceId _instance_id, const BDDNode *_node, klee::ref<klee::Expr> _condition)
      : TofinoModule(ModuleType(ModuleCategory::Tofino_ParserCondition, _instance_id), "ParserCondition", _node), condition(_condition) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    ParserCondition *cloned = new ParserCondition(get_type().instance_id, node, condition);
    return cloned;
  }

  klee::ref<klee::Expr> get_condition() const { return condition; }
};

class ParserConditionFactory : public TofinoModuleFactory {
public:
  ParserConditionFactory(const InstanceId _instance_id)
      : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_ParserCondition, _instance_id), "ParserCondition") {}

  static std::vector<parser_selection_t> build_parser_select(klee::ref<klee::Expr> condition);

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
