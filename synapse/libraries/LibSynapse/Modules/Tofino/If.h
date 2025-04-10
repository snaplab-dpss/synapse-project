#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class If : public TofinoModule {
private:
  klee::ref<klee::Expr> original_condition;

  // AND-changed conditions.
  // Every condition must be met to be equivalent to the original condition.
  // This will later be split into multiple branch conditions, each evaluating
  // each sub condition.
  std::vector<klee::ref<klee::Expr>> conditions;

public:
  If(const LibBDD::Node *node, klee::ref<klee::Expr> _original_condition, const std::vector<klee::ref<klee::Expr>> &_conditions)
      : TofinoModule(ModuleType::Tofino_If, "If", node), original_condition(_original_condition), conditions(_conditions) {}

  If(const LibBDD::Node *node, klee::ref<klee::Expr> _original_condition)
      : TofinoModule(ModuleType::Tofino_If, "If", node), original_condition(_original_condition), conditions({_original_condition}) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    If *cloned = new If(node, original_condition, conditions);
    return cloned;
  }

  klee::ref<klee::Expr> get_original_condition() const { return original_condition; }
  const std::vector<klee::ref<klee::Expr>> &get_conditions() const { return conditions; }
};

class IfFactory : public TofinoModuleFactory {
public:
  IfFactory() : TofinoModuleFactory(ModuleType::Tofino_If, "If") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse