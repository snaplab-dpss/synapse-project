#pragma once

#include "tofino_module.h"

namespace synapse {
namespace tofino {

class SendToController : public TofinoModule {
private:
  Symbols symbols;

public:
  SendToController(const Node *node, Symbols _symbols)
      : TofinoModule(ModuleType::Tofino_SendToController, TargetType::Controller, "SendToController", node), symbols(_symbols) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    SendToController *cloned = new SendToController(node, symbols);
    return cloned;
  }

  const Symbols &get_symbols() const { return symbols; }
};

class SendToControllerFactory : public TofinoModuleFactory {
public:
  SendToControllerFactory() : TofinoModuleFactory(ModuleType::Tofino_SendToController, "SendToController") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node, SymbolManager *symbol_manager) const override;
};

} // namespace tofino
} // namespace synapse