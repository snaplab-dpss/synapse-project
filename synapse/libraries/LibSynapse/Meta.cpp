#include <LibSynapse/Meta.h>
#include <LibSynapse/Modules/Module.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {

void EPMeta::process_node(const BDDNode *node, TargetType target) {
  if (processed_nodes.find(node->get_id()) == processed_nodes.end()) {
    processed_nodes.insert(node->get_id());
  }
}

void EPMeta::update(const EPLeaf &leaf, const EPNode *new_node, bool should_process_node) {
  const ep_node_id_t node_id = new_node->get_id();
  if (visited_ep_nodes.find(node_id) != visited_ep_nodes.end()) {
    return;
  }

  visited_ep_nodes.insert(node_id);

  const Module *module    = new_node->get_module();
  const TargetType target = module->get_target();
  const ModuleType type   = module->get_type();

  auto module_counter_it = modules_counter.find(type.type);
  if (module_counter_it == modules_counter.end()) {
    modules_counter[type.type] = 1;
  } else {
    module_counter_it->second++;
  }

  auto steps_per_target_it = steps_per_target.find(target.type);
  if (steps_per_target_it == steps_per_target.end()) {
    steps_per_target[target.type] = 1;
  } else {
    steps_per_target_it->second++;
  }

  if (leaf.node) {
    const ep_node_id_t leaf_id = leaf.node->get_id();
    if (processed_leaves.find(leaf_id) == processed_leaves.end()) {
      processed_leaves.insert(leaf_id);
    }
  }

  if (should_process_node) {
    process_node(leaf.next, target);
  }

  nodes++;
}

double EPMeta::get_bdd_progress() const { return processed_nodes.size() / static_cast<double>(total_bdd_nodes); }

void EPMeta::update_total_bdd_nodes(const BDD *bdd) { total_bdd_nodes = bdd->size(); }

} // namespace LibSynapse
