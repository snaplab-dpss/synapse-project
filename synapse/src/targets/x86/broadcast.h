#pragma once

#include "x86_module.h"

namespace x86 {

class Broadcast : public x86Module {
public:
  Broadcast(const Node *node) : x86Module(ModuleType::x86_Broadcast, "Broadcast", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    Broadcast *cloned = new Broadcast(node);
    return cloned;
  }
};

class BroadcastFactory : public x86ModuleFactory {
public:
  BroadcastFactory() : x86ModuleFactory(ModuleType::x86_Broadcast, "Broadcast") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace x86
