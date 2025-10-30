#include <LibSynapse/Modules/ModuleFactory.h>
#include <LibSynapse/Target.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibBDD/Reorder.h>
#include <LibBDD/Visitors/BDDVisualizer.h>
#include <LibCore/Expr.h>
#include <LibCore/Debug.h>

namespace LibSynapse {

using LibBDD::anchor_info_t;
using LibBDD::bdd_node_id_t;
using LibBDD::BDDNode;
using LibBDD::BDDNodeType;
using LibBDD::Branch;
using LibBDD::reorder_op_t;
using LibBDD::reordered_bdd_t;
using LibBDD::symbol_translation_t;

using LibCore::dbg_mode_active;

namespace {
bool can_process_platform(const EP *ep, TargetType target) {
  TargetType current_target = ep->get_active_target();

  return current_target == target;
}

void build_node_translations(translator_t &next_nodes_translator, translator_t &processed_nodes_translator, const BDD *old_bdd,
                             const reorder_op_t &op) {
  next_nodes_translator[op.evicted_id] = op.candidate_info.id;

  for (bdd_node_id_t sibling_id : op.candidate_info.siblings) {
    if (sibling_id == op.candidate_info.id) {
      continue;
    }

    processed_nodes_translator[sibling_id] = op.candidate_info.id;

    const BDDNode *sibling = old_bdd->get_node_by_id(sibling_id);
    assert(sibling->get_type() != BDDNodeType::Branch && "Branches not supported");

    const BDDNode *replacement = sibling->get_next();

    if (!replacement) {
      continue;
    }

    next_nodes_translator[sibling_id] = replacement->get_id();
  }
}

anchor_info_t get_anchor_info(const EP *ep) {
  const BDDNode *next = ep->get_next_node();
  assert(next && "Next node not found");

  const BDDNode *anchor = next->get_prev();
  assert(anchor && "Anchor node not found");

  if (anchor->get_type() != BDDNodeType::Branch) {
    return {anchor->get_id(), true};
  }

  const Branch *branch = dynamic_cast<const Branch *>(anchor);

  if (branch->get_on_true() == next) {
    return {anchor->get_id(), true};
  }

  assert(branch->get_on_false() == next && "Next node not found in the branch");
  return {anchor->get_id(), false};
}

std::vector<std::unique_ptr<EP>> get_reordered(const EP *ep) {
  const BDDNode *next = ep->get_next_node();
  if (!next) {
    return {};
  }

  const BDDNode *node = next->get_prev();
  if (!node) {
    return {};
  }

  const anchor_info_t anchor_info     = get_anchor_info(ep);
  const BDD *bdd                      = ep->get_bdd();
  const bool allow_shape_altering_ops = false;

  std::vector<std::unique_ptr<EP>> reordered;
  for (reordered_bdd_t &new_bdd : reorder(bdd, anchor_info, allow_shape_altering_ops)) {
    const bool is_ancestor     = false;
    std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep, is_ancestor);

    EP::translation_data_t translation_data{
        .reordered_node             = new_bdd.bdd->get_node_by_id(new_bdd.op.candidate_info.id),
        .next_nodes_translator      = {},
        .processed_nodes_translator = {},
    };

    build_node_translations(translation_data.next_nodes_translator, translation_data.processed_nodes_translator, bdd, new_bdd.op);
    assert(!new_bdd.op2.has_value() && "Not supported");

    new_ep->replace_bdd(std::move(new_bdd.bdd), translation_data);

    if (dbg_mode_active) {
      new_ep->assert_integrity();
    }

    reordered.push_back(std::move(new_ep));
  }

  return reordered;
}
} // namespace

decision_t ModuleFactory::decide(const EP *ep, const BDDNode *node, std::unordered_map<std::string, i32> params) const {
  return decision_t(ep, node->get_id(), type, params);
}

impl_t ModuleFactory::implement(const EP *ep, const BDDNode *node, std::unique_ptr<EP> result, std::unordered_map<std::string, i32> params) const {
  return impl_t(decide(ep, node, params), std::move(result), false);
}

std::vector<impl_t> ModuleFactory::implement(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager, bool reorder_bdd) const {
  if (!can_process_platform(ep, target)) {
    return {};
  }

  std::vector<impl_t> implementations;
  for (impl_t &internal_decision : process_node(ep, node, symbol_manager)) {
    if (dbg_mode_active) {
      std::cout << "Current ModuleFactory: " << name << "\n";
      internal_decision.result->assert_integrity();
    }
    implementations.push_back(std::move(internal_decision));
  }

  if (!reorder_bdd) {
    return implementations;
  }

  // std::cerr << "#Implementations: " << implementations.size() << "\n";

  std::vector<impl_t> reordered_implementations;
  for (const impl_t &impl : implementations) {
    for (std::unique_ptr<EP> &reordered_ep : get_reordered(impl.result.get())) {
      impl_t new_implementation(impl.decision, std::move(reordered_ep), true);
      reordered_implementations.push_back(std::move(new_implementation));
    }
  }

  // std::cerr << "#Reordered Implementations: " << reordered_implementations.size() << "\n";

  std::vector<impl_t> final_implementations;
  final_implementations.reserve(implementations.size() + reordered_implementations.size());
  final_implementations.insert(final_implementations.end(), std::make_move_iterator(implementations.begin()),
                               std::make_move_iterator(implementations.end()));
  final_implementations.insert(final_implementations.end(), std::make_move_iterator(reordered_implementations.begin()),
                               std::make_move_iterator(reordered_implementations.end()));

  // std::cerr << "#Final Implementations: " << final_implementations.size() << "\n";

  return final_implementations;
}

} // namespace LibSynapse
