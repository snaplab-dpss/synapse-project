#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class LPMForward : public TofinoModule {
public:
  LPMForward(const LibBDD::Node *node) : TofinoModule(ModuleType::Tofino_LPMForward, "LPMForward", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new LPMForward(node);
    return cloned;
  }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {}; }
};

class LPMForwardFactory : public TofinoModuleFactory {
public:
  LPMForwardFactory() : TofinoModuleFactory(ModuleType::Tofino_LPMForward, "LPMForward") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
};

} // namespace Tofino
} // namespace LibSynapse