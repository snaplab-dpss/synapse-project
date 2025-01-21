#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class Then : public ControllerModule {
public:
  Then(const LibBDD::Node *node) : ControllerModule(ModuleType::Controller_Then, "Then", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Then *cloned = new Then(node);
    return cloned;
  }
};

class ThenFactory : public ControllerModuleFactory {
public:
  ThenFactory() : ControllerModuleFactory(ModuleType::Controller_Then, "Then") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
};

} // namespace Controller
} // namespace LibSynapse