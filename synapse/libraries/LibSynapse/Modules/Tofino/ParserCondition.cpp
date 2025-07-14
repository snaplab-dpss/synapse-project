#include <LibSynapse/Modules/Tofino/ParserCondition.h>
#include <LibSynapse/Modules/Tofino/If.h>
#include <LibSynapse/Modules/Tofino/Else.h>
#include <LibSynapse/Modules/Tofino/Then.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Branch;
using LibBDD::Call;
using LibBDD::call_t;

using LibCore::filter;
using LibCore::is_constant;
using LibCore::is_readLSB;
using LibCore::simplify_conditional;

void add_selection(std::vector<parser_selection_t> &selections, const parser_selection_t &new_selection) {
  for (parser_selection_t &selection : selections) {
    if ((selection.target.isNull() && new_selection.target.isNull()) ||
        (solver_toolbox.are_exprs_always_equal(selection.target, new_selection.target))) {
      selection.values.insert(selection.values.end(), new_selection.values.begin(), new_selection.values.end());
      return;
    }
  }

  selections.push_back(new_selection);
}

void add_selections(std::vector<parser_selection_t> &selections, const std::vector<parser_selection_t> &new_selections) {
  for (const parser_selection_t &new_selection : new_selections) {
    add_selection(selections, new_selection);
  }
}

std::vector<parser_selection_t> ParserConditionFactory::build_parser_select(klee::ref<klee::Expr> condition) {
  condition = filter(condition, {"packet_chunks", "DEVICE"});
  condition = simplify_conditional(condition);

  parser_selection_t selection;

  switch (condition->getKind()) {
  case klee::Expr::Kind::Or: {
    klee::ref<klee::Expr> lhs = condition->getKid(0);
    klee::ref<klee::Expr> rhs = condition->getKid(1);

    const std::vector<parser_selection_t> lhs_sel = build_parser_select(lhs);
    const std::vector<parser_selection_t> rhs_sel = build_parser_select(rhs);

    std::vector<parser_selection_t> selections;
    add_selections(selections, lhs_sel);
    add_selections(selections, rhs_sel);
    return selections;
  } break;
  case klee::Expr::Kind::Ne:
    selection.negated = true;
    [[fallthrough]];
  case klee::Expr::Kind::Eq: {
    klee::ref<klee::Expr> lhs = condition->getKid(0);
    klee::ref<klee::Expr> rhs = condition->getKid(1);

    const bool lhs_is_readLSB = is_readLSB(lhs);
    const bool rhs_is_readLSB = is_readLSB(rhs);

    assert_or_panic((lhs_is_readLSB || rhs_is_readLSB), "Not implemented");
    assert_or_panic((lhs_is_readLSB != rhs_is_readLSB), "Not implemented");

    klee::ref<klee::Expr> target = lhs_is_readLSB ? lhs : rhs;
    assert((selection.target.isNull() || solver_toolbox.are_exprs_always_equal(selection.target, target)) && "Not implemented");
    if (selection.target.isNull()) {
      selection.target = target;
    }

    const bool lhs_is_target = solver_toolbox.are_exprs_always_equal(lhs, target);
    const bool rhs_is_target = solver_toolbox.are_exprs_always_equal(rhs, target);
    assert_or_panic((lhs_is_target || rhs_is_target), "Not implemented");

    klee::ref<klee::Expr> value_expr = lhs_is_target ? rhs : lhs;
    assert_or_panic(is_constant(value_expr), "Not implemented");

    selection.values.push_back(value_expr);
  } break;
  default: {
    panic("Parser condition not implemented: %s", expr_to_string(condition).c_str());
  }
  }

  return {selection};
}

std::optional<spec_impl_t> ParserConditionFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Branch) {
    return {};
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (!branch_node->is_parser_condition()) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> ParserConditionFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Branch) {
    return {};
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (!branch_node->is_parser_condition()) {
    return {};
  }

  klee::ref<klee::Expr> original_condition = branch_node->get_condition();

  const BDDNode *on_true  = branch_node->get_on_true();
  const BDDNode *on_false = branch_node->get_on_false();

  assert(on_true && "Branch node without on_true");
  assert(on_false && "Branch node without on_false");

  const std::vector<const Call *> on_true_borrows  = on_true->get_future_functions({"packet_borrow_next_chunk"}, true);
  const std::vector<const Call *> on_false_borrows = on_false->get_future_functions({"packet_borrow_next_chunk"}, true);

  // We are working under the assumption that before parsing a header we always perform some kind of checking.
  if (!(on_true_borrows.size() > 0 || on_false_borrows.size() > 0)) {
    not_implemented();
  }

  if (on_true_borrows.size() != on_false_borrows.size()) {
    const BDDNode *conditional_borrow   = on_true_borrows.size() > on_false_borrows.size() ? on_true_borrows[0] : on_false_borrows[0];
    const BDDNode *not_conditional_path = on_true_borrows.size() > on_false_borrows.size() ? on_false : on_true;

    // Missing implementation of discriminating parsing between multiple headers.
    // Right now we are assuming that either we parse the target header, or we drop the packet.
    if (!not_conditional_path->is_packet_drop_code_path()) {
      not_implemented();
    }

    // Relevant for IPv4 options, but left for future work.
    if (conditional_borrow->get_type() == BDDNodeType::Call) {
      const Call *cb = dynamic_cast<const Call *>(conditional_borrow);
      if (cb->is_hdr_parse_with_var_len()) {
        not_implemented();
      }
    }
  }

  assert(branch_node->get_on_true() && "Branch node without on_true");
  assert(branch_node->get_on_false() && "Branch node without on_false");

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Module *if_module   = new ParserCondition(node, original_condition);
  Module *then_module = new Then(node);
  Module *else_module = new Else(node);

  EPNode *if_node   = new EPNode(if_module);
  EPNode *then_node = new EPNode(then_module);
  EPNode *else_node = new EPNode(else_module);

  if_node->set_children(original_condition, then_node, else_node);
  then_node->set_prev(if_node);
  else_node->set_prev(if_node);

  EPLeaf then_leaf(then_node, branch_node->get_on_true());
  EPLeaf else_leaf(else_node, branch_node->get_on_false());

  new_ep->process_leaf(if_node, {then_leaf, else_leaf});

  return {};
}

std::unique_ptr<Module> ParserConditionFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Branch) {
    return {};
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (!branch_node->is_parser_condition()) {
    return {};
  }

  klee::ref<klee::Expr> condition = branch_node->get_condition();
  return std::make_unique<ParserCondition>(node, condition);
}

} // namespace Tofino
} // namespace LibSynapse