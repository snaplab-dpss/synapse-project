#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

// Hand the packet over to the egress pipeline. Everything after this point in the plan is
// emitted into the egress control, and the values the future still needs travel there in a
// header, exactly as a recirculation carries them.
//
// Unlike a recirculation this is nearly free: it is the same packet on the same pass, and it
// buys a second 20-stage dependency budget. What it cannot do is pick a port, because
// ucast_egress_port is written in ingress, so the forwarding decision has to be made before the
// cut. Dropping in egress is still allowed.
class SendToEgress : public TofinoModule {
private:
  Symbols symbols;

public:
  SendToEgress(const BDDNode *_node, Symbols _symbols)
      : TofinoModule(ModuleType::Tofino_SendToEgress, "SendToEgress", _node), symbols(_symbols) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const { return new SendToEgress(node, symbols); }

  const Symbols &get_symbols() const { return symbols; }
};

class SendToEgressFactory : public TofinoModuleFactory {
public:
  SendToEgressFactory() : TofinoModuleFactory(ModuleType::Tofino_SendToEgress, "SendToEgress") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
