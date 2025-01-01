#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class Forward : public TofinoCPUModule {
private:
  int dst_device;

public:
  Forward(const Node *node, int _dst_device)
      : TofinoCPUModule(ModuleType::TofinoCPU_Forward, "Forward", node),
        dst_device(_dst_device) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Forward *cloned = new Forward(node, dst_device);
    return cloned;
  }

  int get_dst_device() const { return dst_device; }
};

class ForwardFactory : public TofinoCPUModuleFactory {
public:
  ForwardFactory() : TofinoCPUModuleFactory(ModuleType::TofinoCPU_Forward, "Forward") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino_cpu
