#include "module_generator.h"

#include "../execution_plan/execution_plan.h"
#include "../log.h"

static bool can_process_platform(const EP *ep, TargetType target) {
  TargetType current_target = ep->get_current_platform();
  return current_target == target;
}

static void build_node_translations(translator_t &next_nodes_translator,
                                    translator_t &processed_nodes_translator,
                                    const BDD *old_bdd,
                                    const reorder_op_t &op) {
  next_nodes_translator[op.evicted_id] = op.candidate_info.id;

  for (node_id_t sibling_id : op.candidate_info.siblings) {
    if (sibling_id == op.candidate_info.id) {
      continue;
    }

    processed_nodes_translator[sibling_id] = op.candidate_info.id;

    const Node *sibling = old_bdd->get_node_by_id(sibling_id);
    assert(sibling->get_type() != NodeType::BRANCH);

    const Node *replacement = sibling->get_next();

    if (!replacement) {
      continue;
    }

    next_nodes_translator[sibling_id] = replacement->get_id();
  }
}

static anchor_info_t get_anchor_info(const EP *ep) {
  const Node *next = ep->get_next_node();
  assert(next);

  const Node *anchor = next->get_prev();
  assert(anchor);

  if (anchor->get_type() != NodeType::BRANCH) {
    return {anchor->get_id(), true};
  }

  const Branch *branch = static_cast<const Branch *>(anchor);

  if (branch->get_on_true() == next) {
    return {anchor->get_id(), true};
  }

  assert(branch->get_on_false() == next);
  return {anchor->get_id(), false};
}

static std::vector<EP *> get_reordered(const EP *ep) {
  std::vector<EP *> reordered;

  const Node *next = ep->get_next_node();

  if (!next) {
    return reordered;
  }

  const Node *node = next->get_prev();

  if (!node) {
    return reordered;
  }

  anchor_info_t anchor_info = get_anchor_info(ep);

  const BDD *bdd = ep->get_bdd();
  bool allow_shape_altering_ops = false;

  std::vector<reordered_bdd_t> new_bdds =
      reorder(bdd, anchor_info, allow_shape_altering_ops);

  for (const reordered_bdd_t &new_bdd : new_bdds) {
    bool is_ancestor = false;
    EP *new_ep = new EP(*ep, is_ancestor);

    translator_t next_nodes_translator;
    translator_t processed_nodes_translator;

    build_node_translations(next_nodes_translator, processed_nodes_translator,
                            bdd, new_bdd.op);
    assert(!new_bdd.op2.has_value());

    new_ep->replace_bdd(new_bdd.bdd, next_nodes_translator,
                        processed_nodes_translator);
    // new_ep->inspect();

    reordered.push_back(new_ep);
  }

  return reordered;
}

decision_t
ModuleGenerator::decide(const EP *ep, const Node *node,
                        std::unordered_map<std::string, i32> params) const {
  return decision_t(ep, node->get_id(), type, params);
}

impl_t
ModuleGenerator::implement(const EP *ep, const Node *node, EP *result,
                           std::unordered_map<std::string, i32> params) const {
  return impl_t(decide(ep, node, params), result, false);
}

std::vector<impl_t> ModuleGenerator::generate(const EP *ep, const Node *node,
                                              bool reorder_bdd) const {
  std::vector<impl_t> implementations;

  if (!can_process_platform(ep, target)) {
    return implementations;
  }

  std::vector<impl_t> internal_decisions = process_node(ep, node);

  for (const impl_t &id : internal_decisions) {
    EP *ep = id.result;
    implementations.push_back(id);
  }

  if (!reorder_bdd) {
    return implementations;
  }

  for (const impl_t &id : internal_decisions) {
    std::vector<EP *> reordered = get_reordered(id.result);

    for (EP *reordered_ep : reordered) {
      impl_t new_implementation = id;
      new_implementation.result = reordered_ep;
      new_implementation.bdd_reordered = true;
      implementations.push_back(new_implementation);
    }
  }

  return implementations;
}

bool ModuleGenerator::can_place(const EP *ep, const Call *call_node,
                                const std::string &obj_arg,
                                PlacementDecision decision) const {
  const call_t &call = call_node->get_call();

  assert(call.args.find(obj_arg) != call.args.end());
  klee::ref<klee::Expr> obj_expr = call.args.at(obj_arg).expr;
  addr_t obj = expr_addr_to_obj_addr(obj_expr);

  const Context &ctx = ep->get_ctx();
  return ctx.can_place(obj, decision);
}

bool ModuleGenerator::check_placement(const EP *ep, const Call *call_node,
                                      const std::string &obj_arg,
                                      PlacementDecision decision) const {
  const call_t &call = call_node->get_call();

  assert(call.args.find(obj_arg) != call.args.end());
  klee::ref<klee::Expr> obj_expr = call.args.at(obj_arg).expr;
  addr_t obj = expr_addr_to_obj_addr(obj_expr);

  const Context &ctx = ep->get_ctx();
  return ctx.check_placement(obj, decision);
}

void ModuleGenerator::place(EP *ep, addr_t obj,
                            PlacementDecision decision) const {
  Context &new_ctx = ep->get_mutable_ctx();
  new_ctx.save_placement(obj, decision);
}