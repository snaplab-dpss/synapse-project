#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

class Broadcast : public ControllerModule {
public:
  Broadcast(const Node *node) : ControllerModule(ModuleType::Controller_Broadcast, "Broadcast", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Broadcast *cloned = new Broadcast(node);
    return cloned;
  }
};

class BroadcastFactory : public ControllerModuleFactory {
public:
  BroadcastFactory() : ControllerModuleFactory(ModuleType::Controller_Broadcast, "Broadcast") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse