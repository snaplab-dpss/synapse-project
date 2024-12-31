#pragma once

#include "tofino_module.h"

namespace tofino {

class HHTableConditionalUpdateGenerator : public TofinoModuleGenerator {
public:
  HHTableConditionalUpdateGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_HHTableConditionalUpdate,
                              "HHTableConditionalUpdate") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override;
};

} // namespace tofino
