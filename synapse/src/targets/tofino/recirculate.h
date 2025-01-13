#pragma once

#include "tofino_module.h"

namespace synapse {
namespace tofino {

class Recirculate : public TofinoModule {
private:
  Symbols symbols;
  int recirc_port;

public:
  Recirculate(const Node *node, Symbols _symbols, int _recirc_port)
      : TofinoModule(ModuleType::Tofino_Recirculate, "Recirculate", node), symbols(_symbols),
        recirc_port(_recirc_port) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Recirculate *cloned = new Recirculate(node, symbols, recirc_port);
    return cloned;
  }

  const Symbols &get_symbols() const { return symbols; }
  int get_recirc_port() const { return recirc_port; }
};

class RecirculateFactory : public TofinoModuleFactory {
public:
  RecirculateFactory() : TofinoModuleFactory(ModuleType::Tofino_Recirculate, "Recirculate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace tofino
} // namespace synapse