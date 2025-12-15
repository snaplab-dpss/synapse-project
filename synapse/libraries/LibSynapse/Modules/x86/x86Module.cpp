#include <LibSynapse/Modules/x86/x86Module.h>
#include <LibSynapse/Modules/x86/x86Context.h>
#include <LibSynapse/Modules/x86/SendToDevice.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace x86 {
Symbols x86ModuleFactory::get_relevant_dataplane_state(const EP *ep, const BDDNode *node, TargetType target) {
  const bdd_node_ids_t &roots = ep->get_target_roots(target);

  Symbols generated_symbols = node->get_prev_symbols(roots);
  // Symbols map_keys          = node->get_prev_map_keys(roots);

  // generated_symbols.add(map_keys);
  generated_symbols.add(ep->get_bdd()->get_device());
  generated_symbols.add(ep->get_bdd()->get_time());

  std::vector<const Module *> prev_s2d_ep_modules = ep->get_previous_s2d_nodes();

  if (!prev_s2d_ep_modules.empty()) {
    const Module *prev_s2d_module = prev_s2d_ep_modules.front();
    assert(prev_s2d_module->get_type().type == ModuleCategory::x86_SendToDevice);

    const x86::SendToDevice *m = dynamic_cast<const x86::SendToDevice *>(prev_s2d_module);
    assert(m && "Failed to cast Module to x86::SendToDevice");

    const Symbols &prev_shared_symbols = m->get_symbols();

    generated_symbols.add(prev_shared_symbols);
  }

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
