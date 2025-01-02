#include "parser_condition.h"

#include "if.h"
#include "then.h"
#include "else.h"

#include "../../exprs/simplifier.h"

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

    ASSERT(solver_toolbox.are_exprs_always_equal(lhs_sel.target, rhs_sel.target),
           "Not implemented");
    ASSERT(selection.target.isNull() ||
               solver_toolbox.are_exprs_always_equal(lhs_sel.target, selection.target),
           "Not implemented");

    selection.target = lhs_sel.target;
    selection.values.insert(selection.values.end(), lhs_sel.values.begin(),
                            lhs_sel.values.end());
    selection.values.insert(selection.values.end(), rhs_sel.values.begin(),
                            rhs_sel.values.end());
  } break;
  case klee::Expr::Kind::Ne:
    selection.negated = true;
    [[fallthrough]];
  case klee::Expr::Kind::Eq: {
    klee::ref<klee::Expr> lhs = condition->getKid(0);
    klee::ref<klee::Expr> rhs = condition->getKid(1);

    bool lhs_is_readLSB = is_readLSB(lhs);
    bool rhs_is_readLSB = is_readLSB(rhs);

    ASSERT(lhs_is_readLSB || rhs_is_readLSB, "Not implemented");
    ASSERT(lhs_is_readLSB != rhs_is_readLSB, "Not implemented");

    klee::ref<klee::Expr> target = lhs_is_readLSB ? lhs : rhs;
    ASSERT(selection.target.isNull() ||
               solver_toolbox.are_exprs_always_equal(selection.target, target),
           "Not implemented");
    if (selection.target.isNull()) {
      selection.target = target;
    }

    bool lhs_is_target = solver_toolbox.are_exprs_always_equal(lhs, target);
    bool rhs_is_target = solver_toolbox.are_exprs_always_equal(rhs, target);
    ASSERT(lhs_is_target || rhs_is_target, "Not implemented");

    klee::ref<klee::Expr> value_expr = lhs_is_target ? rhs : lhs;
    ASSERT(value_expr->getKind() == klee::Expr::Kind::Constant, "Not implemented");

    selection.values.push_back(solver_toolbox.value_from_expr(value_expr));
  } break;
  case klee::Expr::Kind::Not: {
    klee::ref<klee::Expr> kid = condition->getKid(0);
    selection = build_parser_select(kid);
    selection.negated = !selection.negated;
  } break;
  default: {
    ASSERT(false, "Not implemented");
  }
  }

  return selection;
}
} // namespace

std::optional<spec_impl_t> ParserConditionFactory::speculate(const EP *ep,
                                                             const Node *node,
                                                             const Context &ctx) const {
  if (node->get_type() != NodeType::Branch) {
    return std::nullopt;
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (!is_parser_condition(branch_node)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> ParserConditionFactory::process_node(const EP *ep,
                                                         const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Branch) {
    return impls;
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (!is_parser_condition(branch_node)) {
    return impls;
  }

  klee::ref<klee::Expr> original_condition = branch_node->get_condition();
  selection_t selection = build_parser_select(original_condition);

  const Node *on_true = branch_node->get_on_true();
  const Node *on_false = branch_node->get_on_false();

  ASSERT(on_true, "Branch node without on_true");
  ASSERT(on_false, "Branch node without on_false");

  std::vector<const Call *> on_true_borrows =
      get_future_functions(on_true, {"packet_borrow_next_chunk"}, true);
  std::vector<const Call *> on_false_borrows =
      get_future_functions(on_false, {"packet_borrow_next_chunk"}, true);

  // We are working under the assumption that before parsing a header we
  // always perform some kind of checking.
  ASSERT(on_true_borrows.size() > 0 || on_false_borrows.size() > 0, "Not implemented");

  if (on_true_borrows.size() != on_false_borrows.size()) {
    const Node *conditional_borrow = on_true_borrows.size() > on_false_borrows.size()
                                         ? on_true_borrows[0]
                                         : on_false_borrows[0];
    const Node *not_conditional_path =
        on_true_borrows.size() > on_false_borrows.size() ? on_false : on_true;

    // Missing implementation of discriminating parsing between multiple
    // headers.
    // Right now we are assuming that either we parse the target header, or we
    // drop the packet.
    ASSERT(is_parser_drop(not_conditional_path), "Not implemented");

    // Relevant for IPv4 options, but left for future work.
    ASSERT(!borrow_has_var_len(conditional_borrow), "Not implemented");
  }

  ASSERT(branch_node->get_on_true(), "Branch node without on_true");
  ASSERT(branch_node->get_on_false(), "Branch node without on_false");

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
  tofino_ctx->parser_select(ep, node, selection.target, selection.values,
                            selection.negated);

  return impls;
}

} // namespace tofino
} // namespace synapse