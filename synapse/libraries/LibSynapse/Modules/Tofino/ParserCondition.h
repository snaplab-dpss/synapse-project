#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class ParserCondition : public TofinoModule {
private:
  klee::ref<klee::Expr> condition;

public:
  ParserCondition(const LibBDD::Node *node, klee::ref<klee::Expr> _condition)
      : TofinoModule(ModuleType::Tofino_ParserCondition, "ParserCondition", node), condition(_condition) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    ParserCondition *cloned = new ParserCondition(node, condition);
    return cloned;
  }

  klee::ref<klee::Expr> get_condition() const { return condition; }
};

class ParserConditionFactory : public TofinoModuleFactory {
public:
  ParserConditionFactory() : TofinoModuleFactory(ModuleType::Tofino_ParserCondition, "ParserCondition") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
};

} // namespace Tofino
} // namespace LibSynapse