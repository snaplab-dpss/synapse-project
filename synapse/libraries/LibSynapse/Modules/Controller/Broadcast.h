#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class Broadcast : public ControllerModule {
public:
  Broadcast(const BDDNode *_node) : ControllerModule(ModuleType::Controller_Broadcast, "Broadcast", _node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    Broadcast *cloned = new Broadcast(node);
    return cloned;
  }
};

class BroadcastFactory : public ControllerModuleFactory {
public:
  BroadcastFactory() : ControllerModuleFactory(ModuleType::Controller_Broadcast, "Broadcast") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse