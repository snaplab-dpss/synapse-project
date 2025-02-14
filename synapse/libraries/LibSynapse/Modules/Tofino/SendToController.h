#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class SendToController : public TofinoModule {
private:
  LibCore::Symbols symbols;

public:
  SendToController(const LibBDD::Node *node, LibCore::Symbols _symbols)
      : TofinoModule(ModuleType::Tofino_SendToController, TargetType::Controller, "SendToController", node), symbols(_symbols) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    SendToController *cloned = new SendToController(node, symbols);
    return cloned;
  }

  const LibCore::Symbols &get_symbols() const { return symbols; }
};

class SendToControllerFactory : public TofinoModuleFactory {
public:
  SendToControllerFactory() : TofinoModuleFactory(ModuleType::Tofino_SendToController, "SendToController") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse