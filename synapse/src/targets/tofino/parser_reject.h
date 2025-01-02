#pragma once

#include "tofino_module.h"

namespace synapse {
namespace tofino {

class ParserReject : public TofinoModule {
public:
  ParserReject(const Node *node)
      : TofinoModule(ModuleType::Tofino_ParserReject, "ParserReject", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    ParserReject *cloned = new ParserReject(node);
    return cloned;
  }
};

class ParserRejectFactory : public TofinoModuleFactory {
public:
  ParserRejectFactory()
      : TofinoModuleFactory(ModuleType::Tofino_ParserReject, "ParserReject") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino
} // namespace synapse