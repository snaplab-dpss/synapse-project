#pragma once

#include "tofino_module.hpp"

namespace synapse {
namespace tofino {

class HHTableConditionalUpdateFactory : public TofinoModuleFactory {
public:
  HHTableConditionalUpdateFactory()
      : TofinoModuleFactory(ModuleType::Tofino_HHTableConditionalUpdate, "HHTableConditionalUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace tofino
} // namespace synapse