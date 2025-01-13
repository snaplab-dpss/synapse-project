#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

class Forward : public ControllerModule {
private:
  int dst_device;

public:
  Forward(const Node *node, int _dst_device)
      : ControllerModule(ModuleType::Controller_Forward, "Forward", node), dst_device(_dst_device) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Forward *cloned = new Forward(node, dst_device);
    return cloned;
  }

  int get_dst_device() const { return dst_device; }
};

class ForwardFactory : public ControllerModuleFactory {
public:
  ForwardFactory() : ControllerModuleFactory(ModuleType::Controller_Forward, "Forward") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse