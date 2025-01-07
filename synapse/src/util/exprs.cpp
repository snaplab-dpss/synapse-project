#include <iostream>

#include "exprs.h"
#include "retriever.h"
#include "simplifier.h"
#include "solver.h"
#include "../log.h"

#include <klee/util/ExprVisitor.h>

namespace synapse {

namespace {
struct expr_info_t {
  bool has_allowed;
  bool has_not_allowed;
};

class ExpressionFilter : public klee::ExprVisitor::ExprVisitor {
private:
  std::vector<std::string> allowed_symbols;

public:
  ExpressionFilter(const std::vector<std::string> &_allowed_symbols)
      : ExprVisitor(true), allowed_symbols(_allowed_symbols) {}

  expr_info_t check_symbols(klee::ref<klee::Expr> expr) const {
    expr_info_t info{false, false};

    std::unordered_set<std::string> symbols = get_symbols(expr);

    for (auto s : symbols) {
      auto found_it = std::find(allowed_symbols.begin(), allowed_symbols.end(), s);

      if (found_it != allowed_symbols.end()) {
        info.has_allowed = true;
      } else {
        info.has_not_allowed = true;
      }
    }

    return info;
  }

  // Acts only if either lhs or rhs has **only** allowed symbols, while the
  // other has not allowed symbols. Returns the expression with allowed symbols.
  klee::ref<klee::Expr> pick_filtered_expression(klee::ref<klee::Expr> lhs,
                                                 klee::ref<klee::Expr> rhs) const {
    expr_info_t lhs_info = check_symbols(lhs);
    expr_info_t rhs_info = check_symbols(rhs);

    if (lhs_info.has_allowed && !lhs_info.has_not_allowed && rhs_info.has_not_allowed) {
      return lhs;
    }

    if (rhs_info.has_allowed && !rhs_info.has_not_allowed && lhs_info.has_not_allowed) {
      return rhs;
    }

    return klee::ref<klee::Expr>();
  }

  klee::ExprVisitor::Action visitAnd(const klee::AndExpr &e) {
    if (e.getNumKids() != 2)
      return Action::doChildren();
    klee::ref<klee::Expr> lhs = e.getKid(0);
    klee::ref<klee::Expr> rhs = e.getKid(1);
    klee::ref<klee::Expr> new_expr = pick_filtered_expression(lhs, rhs);
    if (new_expr.isNull())
      return Action::doChildren();
    return Action::changeTo(new_expr);
  }

  klee::ExprVisitor::Action visitAnd(const klee::OrExpr &e) {
    if (e.getNumKids() != 2)
      return Action::doChildren();
    klee::ref<klee::Expr> lhs = e.getKid(0);
    klee::ref<klee::Expr> rhs = e.getKid(1);
    klee::ref<klee::Expr> new_expr = pick_filtered_expression(lhs, rhs);
    if (new_expr.isNull())
      return Action::doChildren();
    return Action::changeTo(new_expr);
  }
};
} // namespace

bool is_readLSB(klee::ref<klee::Expr> expr) {
  std::string symbol;
  return is_readLSB(expr, symbol);
}

bool is_readLSB(klee::ref<klee::Expr> expr, std::string &symbol) {
  SYNAPSE_ASSERT(!expr.isNull(), "Null expr");

  if (expr->getKind() == klee::Expr::Read) {
    return true;
  }

  if (expr->getKind() != klee::Expr::Concat) {
    return false;
  }

  auto groups = get_expr_groups(expr);

  if (groups.empty()) {
    return false;
  }

  const expr_group_t &group = groups[0];
  SYNAPSE_ASSERT(group.has_symbol, "Group has no symbol");
  symbol = group.symbol;

  return true;
}

bool is_packet_readLSB(klee::ref<klee::Expr> expr, bytes_t &offset, int &n_bytes) {
  SYNAPSE_ASSERT(!expr.isNull(), "Null expr");

  if (expr->getKind() == klee::Expr::Read) {
    klee::ReadExpr *read = dyn_cast<klee::ReadExpr>(expr);
    if (read->updates.root->name != "packet_chunks") {
      return false;
    }

    klee::ref<klee::Expr> index = read->index;
    if (index->getKind() == klee::Expr::Constant) {
      klee::ConstantExpr *index_const = dynamic_cast<klee::ConstantExpr *>(index.get());

      offset = index_const->getZExtValue();
      n_bytes = read->getWidth() / 8;

      return true;
    }
  }

  if (expr->getKind() != klee::Expr::Concat) {
    return false;
  }

  std::vector<expr_group_t> groups = get_expr_groups(expr);

  if (groups.size() != 1) {
    return false;
  }

  const expr_group_t &group = groups[0];
  SYNAPSE_ASSERT(group.has_symbol, "Group has no symbol");

  if (group.symbol != "packet_chunks") {
    return false;
  }

  offset = group.offset;
  n_bytes = group.n_bytes;

  return true;
}

bool is_packet_readLSB(klee::ref<klee::Expr> expr) {
  bytes_t offset;
  int n_bytes;
  return is_packet_readLSB(expr, offset, n_bytes);
}

bool is_bool(klee::ref<klee::Expr> expr) {
  SYNAPSE_ASSERT(!expr.isNull(), "Null expr");

  if (expr->getWidth() == 1) {
    return true;
  }

  if (expr->getKind() == klee::Expr::ZExt || expr->getKind() == klee::Expr::SExt ||
      expr->getKind() == klee::Expr::Not) {
    return is_bool(expr->getKid(0));
  }

  if (expr->getKind() == klee::Expr::Or || expr->getKind() == klee::Expr::And) {
    return is_bool(expr->getKid(0)) && is_bool(expr->getKid(1));
  }

  return expr->getKind() == klee::Expr::Eq || expr->getKind() == klee::Expr::Uge ||
         expr->getKind() == klee::Expr::Ugt || expr->getKind() == klee::Expr::Ule ||
         expr->getKind() == klee::Expr::Ult || expr->getKind() == klee::Expr::Sge ||
         expr->getKind() == klee::Expr::Sgt || expr->getKind() == klee::Expr::Sle ||
         expr->getKind() == klee::Expr::Slt;
}

bool is_constant(klee::ref<klee::Expr> expr) {
  if (expr->getKind() == klee::Expr::Kind::Constant) {
    return true;
  }

  auto value = solver_toolbox.value_from_expr(expr);
  auto const_value = solver_toolbox.exprBuilder->Constant(value, expr->getWidth());
  auto is_always_eq = solver_toolbox.are_exprs_always_equal(const_value, expr);

  return is_always_eq;
}

bool is_constant_signed(klee::ref<klee::Expr> expr) {
  auto size = expr->getWidth();

  if (!is_constant(expr)) {
    return false;
  }

  SYNAPSE_ASSERT(size <= 64, "Size too big");

  auto value = solver_toolbox.value_from_expr(expr);
  auto sign_bit = value >> (size - 1);

  return sign_bit == 1;
}

int64_t get_constant_signed(klee::ref<klee::Expr> expr) {
  if (!is_constant_signed(expr)) {
    return false;
  }

  u64 value;
  auto width = expr->getWidth();

  if (expr->getKind() == klee::Expr::Kind::Constant) {
    auto constant = dynamic_cast<klee::ConstantExpr *>(expr.get());
    SYNAPSE_ASSERT(width <= 64, "Width too big");
    value = constant->getZExtValue(width);
  } else {
    value = solver_toolbox.value_from_expr(expr);
  }

  u64 mask = 0;
  for (u64 i = 0u; i < width; i++) {
    mask <<= 1;
    mask |= 1;
  }

  return -((~value + 1) & mask);
}

std::optional<std::string> get_symbol(klee::ref<klee::Expr> expr) {
  std::optional<std::string> symbol;

  std::unordered_set<std::string> symbols = get_symbols(expr);

  if (symbols.size() == 1) {
    symbol = *symbols.begin();
  }

  return symbol;
}

bool manager_contains(const klee::ConstraintManager &constraints, klee::ref<klee::Expr> expr) {
  auto found_it =
      std::find_if(constraints.begin(), constraints.end(), [&](klee::ref<klee::Expr> e) {
        return solver_toolbox.are_exprs_always_equal(e, expr);
      });
  return found_it != constraints.end();
}

klee::ConstraintManager join_managers(const klee::ConstraintManager &m1,
                                      const klee::ConstraintManager &m2) {
  klee::ConstraintManager m;

  for (klee::ref<klee::Expr> c : m1) {
    m.addConstraint(c);
  }

  for (klee::ref<klee::Expr> c : m2) {
    if (!manager_contains(m, c))
      m.addConstraint(c);
  }

  return m;
}

addr_t expr_addr_to_obj_addr(klee::ref<klee::Expr> obj_addr) {
  SYNAPSE_ASSERT(!obj_addr.isNull(), "Null obj_addr");
  SYNAPSE_ASSERT(is_constant(obj_addr), "Non-constant obj_addr");
  return solver_toolbox.value_from_expr(obj_addr);
}

klee::ref<klee::Expr> constraint_from_expr(klee::ref<klee::Expr> expr) {
  klee::ref<klee::Expr> constraint;

  switch (expr->getKind()) {
  // Comparisons
  case klee::Expr::Eq:
  case klee::Expr::Ne:
  case klee::Expr::Ult:
  case klee::Expr::Ule:
  case klee::Expr::Ugt:
  case klee::Expr::Uge:
  case klee::Expr::Slt:
  case klee::Expr::Sle:
  case klee::Expr::Sgt:
  case klee::Expr::Sge:
    constraint = expr;
    break;
  // Constant
  case klee::Expr::Constant:
  // Reads
  case klee::Expr::Read:
  case klee::Expr::Select:
  case klee::Expr::Concat:
  case klee::Expr::Extract:
  // Bit
  case klee::Expr::Not:
  case klee::Expr::And:
  case klee::Expr::Or:
  case klee::Expr::Xor:
  case klee::Expr::Shl:
  case klee::Expr::LShr:
  case klee::Expr::AShr:
  // Casting
  case klee::Expr::ZExt:
  case klee::Expr::SExt:
  // Arithmetic
  case klee::Expr::Add:
  case klee::Expr::Sub:
  case klee::Expr::Mul:
  case klee::Expr::UDiv:
  case klee::Expr::SDiv:
  case klee::Expr::URem:
  case klee::Expr::SRem:
    constraint = solver_toolbox.exprBuilder->Ne(
        expr, solver_toolbox.exprBuilder->Constant(0, expr->getWidth()));
    break;
  default:
    SYNAPSE_PANIC("TODO");
  }

  return constraint;
}

klee::ref<klee::Expr> filter(klee::ref<klee::Expr> expr,
                             const std::vector<std::string> &allowed_symbols) {
  ExpressionFilter filter(allowed_symbols);

  expr_info_t data = filter.check_symbols(expr);
  if (!data.has_allowed && data.has_not_allowed) {
    return solver_toolbox.exprBuilder->True();
  }

  klee::ref<klee::Expr> filtered = filter.visit(expr);
  SYNAPSE_ASSERT(filter.check_symbols(filtered).has_not_allowed == 0, "Invalid filter");

  return filtered;
}

std::vector<mod_t> build_expr_mods(klee::ref<klee::Expr> before, klee::ref<klee::Expr> after) {
  SYNAPSE_ASSERT(before->getWidth() == after->getWidth(), "Different widths");

  if (after->getKind() == klee::Expr::Concat) {
    bits_t msb_width = after->getKid(0)->getWidth();
    bits_t lsb_width = after->getWidth() - msb_width;

    std::vector<mod_t> msb_groups = build_expr_mods(
        solver_toolbox.exprBuilder->Extract(before, lsb_width, msb_width), after->getKid(0));

    std::vector<mod_t> lsb_groups = build_expr_mods(
        solver_toolbox.exprBuilder->Extract(before, 0, lsb_width), after->getKid(1));

    std::vector<mod_t> groups = lsb_groups;

    for (mod_t group : msb_groups) {
      group.offset += lsb_width / 8;
      groups.push_back(group);
    }

    return groups;
  }

  if (solver_toolbox.are_exprs_always_equal(before, after)) {
    return {};
  }

  return {mod_t(0, after->getWidth(), after)};
}

std::vector<klee::ref<klee::Expr>> bytes_in_expr(klee::ref<klee::Expr> expr) {
  std::vector<klee::ref<klee::Expr>> bytes;

  bytes_t size = expr->getWidth() / 8;
  for (bytes_t i = 0; i < size; i++) {
    bytes.push_back(solver_toolbox.exprBuilder->Extract(expr, i * 8, 8));
  }

  return bytes;
}

} // namespace synapse