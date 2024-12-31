#pragma once

#include "x86_module.h"

namespace x86 {

class Else : public x86Module {
public:
  Else(const Node *node) : x86Module(ModuleType::x86_Else, "Else", node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Else *cloned = new Else(node);
    return cloned;
  }
};

class ElseGenerator : public x86ModuleGenerator {
public:
  ElseGenerator() : x86ModuleGenerator(ModuleType::x86_Else, "Else") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override;
};

} // namespace x86
