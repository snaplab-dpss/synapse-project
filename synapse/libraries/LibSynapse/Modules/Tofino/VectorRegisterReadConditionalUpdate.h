#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class VectorRegisterReadConditionalUpdate : public TofinoModule {
public:
  VectorRegisterReadConditionalUpdate(const BDDNode *_node)
      : TofinoModule(ModuleType::Tofino_VectorRegisterReadConditionalUpdate, "VectorRegisterReadConditionalUpdate", _node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new VectorRegisterReadConditionalUpdate(node);
    return cloned;
  }
};

class VectorRegisterReadConditionalUpdateFactory : public TofinoModuleFactory {
public:
  VectorRegisterReadConditionalUpdateFactory()
      : TofinoModuleFactory(ModuleType::Tofino_VectorRegisterReadConditionalUpdate, "VectorRegisterReadConditionalUpdate") {}

  // Pattern-level check only (no data-structure placement / usage checks): would this
  // module claim `node` (a vector_borrow with a conditional write)? Used by
  // VectorRegisterLookup to decide whether to leave such a borrow to this module.
  static bool matches_pattern(const EP *ep, const BDDNode *node);

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse