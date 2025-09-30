#pragma once

#include "LibSynapse/Target.h"
#include "LibCore/Types.h"
#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {
class SendToDevice : public x86Module {
private:
  u32 outgoing_port;
  u32 incoming_port;
  TargetType next_type;
  Symbols symbols;

public:
  SendToDevice(const std::string &_instance_id, const BDDNode *_node, u16 _outgoing_port, u16 _incoming_port, TargetType _next_type, Symbols _symbols)
      : x86Module(ModuleType(ModuleCategory::x86_SendToDevice, _instance_id), _next_type, "SendToDevice", _node), outgoing_port(_outgoing_port),
        incoming_port(_incoming_port), next_type(_next_type), symbols(_symbols) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new SendToDevice(get_type().instance_id, node, outgoing_port, incoming_port, next_type, symbols); }

  u16 get_outgoing_port() const { return outgoing_port; }
  u16 get_incoming_port() const { return incoming_port; }
  TargetType get_next_type() const { return next_type; }
  const Symbols &get_symbols() const { return symbols; }
};

class SendToDeviceFactory : public x86ModuleFactory {
public:
  SendToDeviceFactory(const std::string &_instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_SendToDevice, _instance_id), "SendToDevice") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse
