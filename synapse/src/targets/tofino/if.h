#pragma once

#include "tofino_module.h"

namespace synapse {
namespace tofino {

class If : public TofinoModule {
private:
  klee::ref<klee::Expr> original_condition;

  // AND-changed conditions.
  // Every condition must be met to be equivalent to the original condition.
  // This will later be split into multiple branch conditions, each evaluating
  // each sub condition.
  std::vector<klee::ref<klee::Expr>> conditions;

public:
  If(const Node *node, klee::ref<klee::Expr> _original_condition,
     const std::vector<klee::ref<klee::Expr>> &_conditions)
      : TofinoModule(ModuleType::Tofino_If, "If", node),
        original_condition(_original_condition), conditions(_conditions) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    If *cloned = new If(node, original_condition, conditions);
    return cloned;
  }

  const std::vector<klee::ref<klee::Expr>> &get_conditions() const { return conditions; }
};

class IfFactory : public TofinoModuleFactory {
public:
  IfFactory() : TofinoModuleFactory(ModuleType::Tofino_If, "If") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino
} // namespace synapse