#include <LibTessera/Modules/x86/Else.h>
#include <LibTessera/ExecutionPlan.h>

namespace LibTessera {
namespace x86 {

std::optional<spec_impl_t> ElseFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

std::vector<impl_t> ElseFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

std::unique_ptr<Module> ElseFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

} // namespace x86
} // namespace LibTessera