#include <LibSynapse/Modules/x86/x86Module.h>
#include <LibSynapse/Modules/x86/x86Context.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace x86 {
Symbols x86ModuleFactory::get_relevant_dataplane_state(const EP *ep, const BDDNode *node, TargetType target) {
  const bdd_node_ids_t &roots = ep->get_target_roots(target);

  Symbols generated_symbols = node->get_prev_symbols(roots);
  generated_symbols.add(ep->get_bdd()->get_device());
  generated_symbols.add(ep->get_bdd()->get_time());

  Symbols future_used_symbols;
  node->visit_nodes([&future_used_symbols](const BDDNode *future_node) {
    const Symbols local_future_symbols = future_node->get_used_symbols();
    future_used_symbols.add(local_future_symbols);
    return BDDNodeVisitAction::Continue;
  });

  return generated_symbols.intersect(future_used_symbols);
}
} // namespace x86
} // namespace LibSynapse