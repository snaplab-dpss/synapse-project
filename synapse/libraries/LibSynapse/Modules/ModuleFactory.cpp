#include <LibSynapse/Modules/ModuleFactory.h>
#include <LibSynapse/Target.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibBDD/Reorder.h>
#include <LibBDD/Visitors/BDDVisualizer.h>
#include <LibCore/Expr.h>
#include <LibCore/Debug.h>

namespace LibSynapse {

namespace {
bool can_process_platform(const EP *ep, TargetType target) {
  TargetType current_target = ep->get_active_target();
  return current_target == target;
}

void build_node_translations(translator_t &next_nodes_translator, translator_t &processed_nodes_translator, const LibBDD::BDD *old_bdd,
                             const LibBDD::reorder_op_t &op) {
  next_nodes_translator[op.evicted_id] = op.candidate_info.id;

  for (LibBDD::node_id_t sibling_id : op.candidate_info.siblings) {
    if (sibling_id == op.candidate_info.id) {
      continue;
    }

    processed_nodes_translator[sibling_id] = op.candidate_info.id;

    const LibBDD::Node *sibling = old_bdd->get_node_by_id(sibling_id);
    assert(sibling->get_type() != LibBDD::NodeType::Branch && "Branches not supported");

    const LibBDD::Node *replacement = sibling->get_next();

    if (!replacement) {
      continue;
    }

    next_nodes_translator[sibling_id] = replacement->get_id();
  }
}

LibBDD::anchor_info_t get_anchor_info(const EP *ep) {
  const LibBDD::Node *next = ep->get_next_node();
  assert(next && "Next node not found");

  const LibBDD::Node *anchor = next->get_prev();
  assert(anchor && "Anchor node not found");

  if (anchor->get_type() != LibBDD::NodeType::Branch) {
    return {anchor->get_id(), true};
  }

  const LibBDD::Branch *branch = dynamic_cast<const LibBDD::Branch *>(anchor);

  if (branch->get_on_true() == next) {
    return {anchor->get_id(), true};
  }

  assert(branch->get_on_false() == next && "Next node not found in the branch");
  return {anchor->get_id(), false};
}

std::vector<std::unique_ptr<EP>> get_reordered(const EP *ep) {
  const LibBDD::Node *next = ep->get_next_node();
  if (!next) {
    return {};
  }

  const LibBDD::Node *node = next->get_prev();
  if (!node) {
    return {};
  }

  const LibBDD::anchor_info_t anchor_info = get_anchor_info(ep);
  const LibBDD::BDD *bdd                  = ep->get_bdd();
  const bool allow_shape_altering_ops     = false;

  std::vector<std::unique_ptr<EP>> reordered;
  for (LibBDD::reordered_bdd_t &new_bdd : reorder(bdd, anchor_info, allow_shape_altering_ops)) {
    const bool is_ancestor     = false;
    std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep, is_ancestor);

    EP::translation_data_t translation_data{
        .reordered_node             = new_bdd.bdd->get_node_by_id(new_bdd.op.candidate_info.id),
        .next_nodes_translator      = {},
        .processed_nodes_translator = {},
        .translated_symbols         = new_bdd.translated_symbols,
    };

    build_node_translations(translation_data.next_nodes_translator, translation_data.processed_nodes_translator, bdd, new_bdd.op);
    assert(!new_bdd.op2.has_value() && "Not supported");

    new_ep->replace_bdd(std::move(new_bdd.bdd), translation_data);
    new_ep->assert_integrity();

    reordered.push_back(std::move(new_ep));
  }

  return reordered;
}
} // namespace

decision_t ModuleFactory::decide(const EP *ep, const LibBDD::Node *node, std::unordered_map<std::string, i32> params) const {
  return decision_t(ep, node->get_id(), type, params);
}

impl_t ModuleFactory::implement(const EP *ep, const LibBDD::Node *node, std::unique_ptr<EP> result,
                                std::unordered_map<std::string, i32> params) const {
  return impl_t(decide(ep, node, params), std::move(result), false);
}

std::vector<impl_t> ModuleFactory::implement(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager, bool reorder_bdd) const {
  if (!can_process_platform(ep, target)) {
    return {};
  }

  std::vector<impl_t> implementations;
  for (impl_t &internal_decision : process_node(ep, node, symbol_manager)) {
    implementations.push_back(std::move(internal_decision));
  }

  if (!reorder_bdd) {
    return implementations;
  }

  const size_t total_internal_decisions = implementations.size();
  for (size_t i = 0; i < total_internal_decisions; ++i) {
    for (std::unique_ptr<EP> &reordered_ep : get_reordered(implementations[i].result.get())) {
      impl_t new_implementation(implementations[i].decision, std::move(reordered_ep), true);
      implementations.push_back(std::move(new_implementation));
    }
  }

  return implementations;
}

} // namespace LibSynapse