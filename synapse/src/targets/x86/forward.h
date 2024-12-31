#pragma once

#include "x86_module.h"

namespace x86 {

class Forward : public x86Module {
private:
  int dst_device;

public:
  Forward(const Node *node, int _dst_device)
      : x86Module(ModuleType::x86_Forward, "Forward", node),
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

class ForwardGenerator : public x86ModuleGenerator {
public:
  ForwardGenerator() : x86ModuleGenerator(ModuleType::x86_Forward, "Forward") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override;
};

} // namespace x86
