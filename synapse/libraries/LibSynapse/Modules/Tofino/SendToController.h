#pragma once

#include "LibSynapse/Target.h"
#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class SendToController : public TofinoModule {
private:
  Symbols symbols;

public:
  SendToController(const InstanceId _instance_id, const BDDNode *_node, Symbols _symbols)
      : TofinoModule(ModuleType(ModuleCategory::Tofino_SendToController, _instance_id), TargetType(TargetArchitecture::Controller, _instance_id),
                     "SendToController", _node),
        symbols(_symbols) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    SendToController *cloned = new SendToController(get_type().instance_id, node, symbols);
    return cloned;
  }

  const Symbols &get_symbols() const { return symbols; }
};

class SendToControllerFactory : public TofinoModuleFactory {
public:
  SendToControllerFactory(const InstanceId _instance_id)
      : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_SendToController, _instance_id), "SendToController") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
