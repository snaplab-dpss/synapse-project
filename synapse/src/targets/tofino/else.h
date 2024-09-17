#pragma once

#include "tofino_module.h"

namespace tofino {

class Else : public TofinoModule {
public:
  Else(const Node *node)
      : TofinoModule(ModuleType::Tofino_Else, "Else", node) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Else *cloned = new Else(node);
    return cloned;
  }
};

class ElseGenerator : public TofinoModuleGenerator {
public:
  ElseGenerator() : TofinoModuleGenerator(ModuleType::Tofino_Else, "Else") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    return std::nullopt;
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    // Never explicitly generate this module from the BDD.
    return {};
  }
};

} // namespace tofino
