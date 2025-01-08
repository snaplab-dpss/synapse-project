#include "parser_condition.h"

#include "if.h"
#include "then.h"
#include "else.h"

#include "../../util/simplifier.h"

namespace synapse {
namespace tofino {
namespace {
struct selection_t {
  klee::ref<klee::Expr> target;
  std::vector<int> values;
  bool negated;

  selection_t() : negated(false) {}
};

selection_t build_parser_select(klee::ref<klee::Expr> condition) {
  condition = filter(condition, {"packet_chunks", "DEVICE"});
  condition = swap_packet_endianness(condition);
  condition = simplify(condition);

  selection_t selection;

  switch (condition->getKind()) {
  case klee::Expr::Kind::Or: {
    klee::ref<klee::Expr> lhs = condition->getKid(0);
    klee::ref<klee::Expr> rhs = condition->getKid(1);

    selection_t lhs_sel = build_parser_select(lhs);
    selection_t rhs_sel = build_parser_select(rhs);

    assert(solver_toolbox.are_exprs_always_equal(lhs_sel.target, rhs_sel.target) &&
           "Not implemented");
    assert((selection.target.isNull() ||
            solver_toolbox.are_exprs_always_equal(lhs_sel.target, selection.target)) &&
           "Not implemented");

    selection.target = lhs_sel.target;
    selection.values.insert(selection.values.end(), lhs_sel.values.begin(), lhs_sel.values.end());
    selection.values.insert(selection.values.end(), rhs_sel.values.begin(), rhs_sel.values.end());
  } break;
  case klee::Expr::Kind::Ne:
    selection.negated = true;
    [[fallthrough]];
  case klee::Expr::Kind::Eq: {
    klee::ref<klee::Expr> lhs = condition->getKid(0);
    klee::ref<klee::Expr> rhs = condition->getKid(1);

    bool lhs_is_readLSB = is_readLSB(lhs);
    bool rhs_is_readLSB = is_readLSB(rhs);

    assert((lhs_is_readLSB || rhs_is_readLSB) && "Not implemented");
    assert((lhs_is_readLSB != rhs_is_readLSB) && "Not implemented");

    klee::ref<klee::Expr> target = lhs_is_readLSB ? lhs : rhs;
    assert((selection.target.isNull() ||
            solver_toolbox.are_exprs_always_equal(selection.target, target)) &&
           "Not implemented");
    if (selection.target.isNull()) {
      selection.target = target;
    }

    bool lhs_is_target = solver_toolbox.are_exprs_always_equal(lhs, target);
    bool rhs_is_target = solver_toolbox.are_exprs_always_equal(rhs, target);
    assert((lhs_is_target || rhs_is_target) && "Not implemented");

    klee::ref<klee::Expr> value_expr = lhs_is_target ? rhs : lhs;
    assert(value_expr->getKind() == klee::Expr::Kind::Constant && "Not implemented");

    selection.values.push_back(solver_toolbox.value_from_expr(value_expr));
  } break;
  case klee::Expr::Kind::Not: {
    klee::ref<klee::Expr> kid = condition->getKid(0);
    selection = build_parser_select(kid);
    selection.negated = !selection.negated;
  } break;
  default: {
    panic("Not implemented");
  }
  }

  return selection;
}
} // namespace

std::optional<spec_impl_t> ParserConditionFactory::speculate(const EP *ep, const Node *node,
                                                             const Context &ctx) const {
  if (node->get_type() != NodeType::Branch) {
    return std::nullopt;
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (!branch_node->is_parser_condition()) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> ParserConditionFactory::process_node(const EP *ep, const Node *node,
                                                         SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Branch) {
    return impls;
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (!branch_node->is_parser_condition()) {
    return impls;
  }

  klee::ref<klee::Expr> original_condition = branch_node->get_condition();
  selection_t selection = build_parser_select(original_condition);

  const Node *on_true = branch_node->get_on_true();
  const Node *on_false = branch_node->get_on_false();

  assert(on_true && "Branch node without on_true");
  assert(on_false && "Branch node without on_false");

  std::vector<const Call *> on_true_borrows =
      on_true->get_future_functions({"packet_borrow_next_chunk"}, true);
  std::vector<const Call *> on_false_borrows =
      on_false->get_future_functions({"packet_borrow_next_chunk"}, true);

  // We are working under the assumption that before parsing a header we
  // always perform some kind of checking.
  assert((on_true_borrows.size() > 0 || on_false_borrows.size() > 0) && "Not implemented");

  if (on_true_borrows.size() != on_false_borrows.size()) {
    const Node *conditional_borrow =
        on_true_borrows.size() > on_false_borrows.size() ? on_true_borrows[0] : on_false_borrows[0];
    const Node *not_conditional_path =
        on_true_borrows.size() > on_false_borrows.size() ? on_false : on_true;

    // Missing implementation of discriminating parsing between multiple
    // headers.
    // Right now we are assuming that either we parse the target header, or we
    // drop the packet.
    assert(not_conditional_path->is_packet_drop_code_path() && "Not implemented");

    // Relevant for IPv4 options, but left for future work.
    if (conditional_borrow->get_type() == NodeType::Call) {
      const Call *cb = dynamic_cast<const Call *>(conditional_borrow);
      assert(!cb->is_hdr_parse_with_var_len() && "Not implemented");
    }
  }

  assert(branch_node->get_on_true() && "Branch node without on_true");
  assert(branch_node->get_on_false() && "Branch node without on_false");

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Module *if_module = new ParserCondition(node, original_condition);
  Module *then_module = new Then(node);
  Module *else_module = new Else(node);

  EPNode *if_node = new EPNode(if_module);
  EPNode *then_node = new EPNode(then_module);
  EPNode *else_node = new EPNode(else_module);

  if_node->set_constraint(original_condition);
  if_node->set_children(then_node, else_node);
  then_node->set_prev(if_node);
  else_node->set_prev(if_node);

  EPLeaf then_leaf(then_node, branch_node->get_on_true());
  EPLeaf else_leaf(else_node, branch_node->get_on_false());

  new_ep->process_leaf(if_node, {then_leaf, else_leaf});

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->parser_select(ep, node, selection.target, selection.values, selection.negated);

  return impls;
}

} // namespace tofino
} // namespace synapse