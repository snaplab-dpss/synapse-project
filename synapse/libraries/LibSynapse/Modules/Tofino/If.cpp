#include <LibSynapse/Modules/Tofino/If.h>
#include <LibSynapse/Modules/Tofino/Then.h>
#include <LibSynapse/Modules/Tofino/Else.h>
#include <LibSynapse/ExecutionPlan.h>

#include <LibCore/Math.h>
#include <LibCore/Expr.h>

#include <klee/util/ExprVisitor.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Branch;
using LibCore::expr_addr_to_obj_addr;
using LibCore::is_power_of_two;
using LibCore::is_readLSB;
using LibCore::simplify_conditional;
using LibCore::symbolic_read_t;
using LibCore::symbolic_reads_t;

namespace {

class PHVBytesRetriever : public klee::ExprVisitor::ExprVisitor {
private:
  symbolic_reads_t symbolic_reads;
  bytes_t used_phv_bytes;

public:
  PHVBytesRetriever() : used_phv_bytes(0) {}

  Action visitRead(const klee::ReadExpr &e) override final {
    assert(e.index->getKind() == klee::Expr::Kind::Constant && "Non-constant index");

    const klee::ConstantExpr *index_const = dynamic_cast<klee::ConstantExpr *>(e.index.get());
    const bytes_t byte                    = index_const->getZExtValue();
    const std::string name                = e.updates.root->name;
    const symbolic_read_t symbolic_read{byte, name};

    if (!symbolic_reads.contains(symbolic_read)) {
      symbolic_reads.insert({byte, name});
      used_phv_bytes += 1;
    }

    return Action::doChildren();
  }

  Action visitExpr(const klee::Expr &e) override final {
    if (e.getKind() == klee::Expr::Kind::Read) {
      return Action::doChildren();
    }

    for (size_t i = 0; i < e.getNumKids(); i++) {
      klee::ref<klee::Expr> kid = e.getKid(i);
      if (kid->getKind() == klee::Expr::Constant) {
        const u64 value = solver_toolbox.value_from_expr(kid);

        // If the value is 1 less than a power of two, the compiler can optimize it to not consume PHV resources.
        if (is_power_of_two(value + 1)) {
          continue;
        }

        const bytes_t width = kid->getWidth() / 8;
        used_phv_bytes += width;
      }
    }

    return Action::doChildren();
  }

  bytes_t get_used_phv_bytes() const { return used_phv_bytes; }
};

std::vector<klee::ref<klee::Expr>> split_condition(klee::ref<klee::Expr> condition) {
  std::vector<klee::ref<klee::Expr>> conditions;

  switch (condition->getKind()) {
  case klee::Expr::Kind::And: {
    klee::ref<klee::Expr> lhs = condition->getKid(0);
    klee::ref<klee::Expr> rhs = condition->getKid(1);

    std::vector<klee::ref<klee::Expr>> lhs_conds = split_condition(lhs);
    std::vector<klee::ref<klee::Expr>> rhs_conds = split_condition(rhs);

    conditions.insert(conditions.end(), lhs_conds.begin(), lhs_conds.end());
    conditions.insert(conditions.end(), rhs_conds.begin(), rhs_conds.end());
  } break;
  case klee::Expr::Kind::Or: {
    panic("TODO: Splitting if condition on an OR");
  } break;
  default: {
    conditions.push_back(condition);
  }
  }

  return conditions;
}

bool is_simple_expr(klee::ref<klee::Expr> condition) {
  bool is_simple = false;

  switch (condition->getKind()) {
  case klee::Expr::Kind::Eq:
  case klee::Expr::Kind::Ne:
  case klee::Expr::Kind::Ult:
  case klee::Expr::Kind::Ule:
  case klee::Expr::Kind::Ugt:
  case klee::Expr::Kind::Uge:
  case klee::Expr::Kind::Slt:
  case klee::Expr::Kind::Sle:
  case klee::Expr::Kind::Sgt:
  case klee::Expr::Kind::Sge: {
    klee::ref<klee::Expr> lhs = condition->getKid(0);
    klee::ref<klee::Expr> rhs = condition->getKid(1);
    is_simple                 = is_simple_expr(lhs) && is_simple_expr(rhs);
  } break;
  case klee::Expr::Kind::Not: {
    is_simple = is_simple_expr(condition->getKid(0));
  } break;
  case klee::Expr::Kind::Concat: {
    is_simple = is_readLSB(condition);
  } break;
  case klee::Expr::Kind::Read:
  case klee::Expr::Kind::Constant: {
    is_simple = true;
  } break;
  default:
    is_simple = false;
  }

  return is_simple;
}

If::phv_limitation_workaround_t get_phv_limitation_workaround(klee::ref<klee::Expr> expr) {
  If::phv_limitation_workaround_t workaround;

  static const std::map<std::pair<klee::Expr::Width, klee::Expr::Kind>, If::ConditionActionHelper> kind_to_action_helper{
      {{klee::Expr::Int32, klee::Expr::Kind::Sle}, If::ConditionActionHelper::CheckSignBitForLessThanOrEqual32b},
      {{klee::Expr::Int32, klee::Expr::Kind::Slt}, If::ConditionActionHelper::CheckSignBitForLessThan32b},
      {{klee::Expr::Int32, klee::Expr::Kind::Sge}, If::ConditionActionHelper::CheckSignBitForGreaterThanOrEqual32b},
      {{klee::Expr::Int32, klee::Expr::Kind::Sgt}, If::ConditionActionHelper::CheckSignBitForGreaterThan32b},
  };

  if (expr->getKind() == klee::Expr::Kind::Not) {
    expr = expr->getKid(0);
  }

  if (expr->getNumKids() != 2) {
    return workaround;
  }

  const klee::Expr::Width width = expr->getKid(0)->getWidth();
  const klee::Expr::Kind kind   = expr->getKind();

  auto found_it = kind_to_action_helper.find({width, kind});
  if (found_it != kind_to_action_helper.end()) {
    workaround.action_helper = found_it->second;
    workaround.lhs           = expr->getKid(0);
    workaround.rhs           = expr->getKid(1);
  }

  return workaround;
}

} // namespace

std::optional<spec_impl_t> IfFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Branch) {
    return {};
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (branch_node->is_parser_condition()) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> IfFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Branch) {
    return {};
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (branch_node->is_parser_condition()) {
    return {};
  }

  klee::ref<klee::Expr> condition = branch_node->get_condition();

  std::vector<If::condition_t> conditions;
  for (klee::ref<klee::Expr> sub_condition : split_condition(condition)) {
    klee::ref<klee::Expr> simplified = simplify_conditional(sub_condition);

    if (!is_simple_expr(simplified)) {
      panic("TODO: deal with this not simple condition: %s", expr_to_string(simplified, true).c_str());
      return {};
    }

    If::phv_limitation_workaround_t phv_limitation_workaround;
    if (!get_tna(ep).condition_meets_phv_limit(sub_condition)) {
      phv_limitation_workaround = get_phv_limitation_workaround(simplified);
      if (phv_limitation_workaround.action_helper == If::ConditionActionHelper::None) {
        panic("TODO: deal with this not compatible condition: %s", expr_to_string(simplified, true).c_str());
      }
    }

    conditions.push_back({simplified, phv_limitation_workaround});
  }

  assert(branch_node->get_on_true() && "Branch node without on_true");
  assert(branch_node->get_on_false() && "Branch node without on_false");

  Module *if_module   = new If(node, condition, conditions);
  Module *then_module = new Then(node);
  Module *else_module = new Else(node);

  EPNode *if_node   = new EPNode(if_module);
  EPNode *then_node = new EPNode(then_module);
  EPNode *else_node = new EPNode(else_module);

  if_node->set_children(condition, then_node, else_node);
  then_node->set_prev(if_node);
  else_node->set_prev(if_node);

  EPLeaf then_leaf(then_node, branch_node->get_on_true());
  EPLeaf else_leaf(else_node, branch_node->get_on_false());

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  new_ep->process_leaf(if_node, {then_leaf, else_leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> IfFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Branch) {
    return {};
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (branch_node->is_parser_condition()) {
    return {};
  }

  klee::ref<klee::Expr> condition = branch_node->get_condition();
  const Tofino::TNA &tna          = ctx.get_target_ctx<TofinoContext>()->get_tna();

  std::vector<If::condition_t> conditions;
  for (klee::ref<klee::Expr> sub_condition : split_condition(condition)) {
    klee::ref<klee::Expr> simplified = simplify_conditional(sub_condition);

    if (!is_simple_expr(simplified)) {
      panic("TODO: deal with this not simple condition: %s", expr_to_string(simplified, true).c_str());
      return {};
    }

    If::phv_limitation_workaround_t phv_limitation_workaround;
    if (!tna.condition_meets_phv_limit(sub_condition)) {
      phv_limitation_workaround = get_phv_limitation_workaround(simplified);
      if (phv_limitation_workaround.action_helper == If::ConditionActionHelper::None) {
        panic("TODO: deal with this incompatible condition: %s", expr_to_string(simplified, true).c_str());
      }
    }

    conditions.push_back({simplified, phv_limitation_workaround});
  }

  return std::make_unique<If>(node, condition, conditions);
}

} // namespace Tofino
} // namespace LibSynapse