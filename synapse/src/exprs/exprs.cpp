#include <iostream>

#include "exprs.h"
#include "retriever.h"
#include "simplifier.h"
#include "solver.h"
#include "../log.h"

namespace synapse {
bool is_readLSB(klee::ref<klee::Expr> expr) {
  std::string symbol;
  return is_readLSB(expr, symbol);
}

bool is_readLSB(klee::ref<klee::Expr> expr, std::string &symbol) {
  ASSERT(!expr.isNull(), "Null expr");

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
  ASSERT(group.has_symbol, "Group has no symbol");
  symbol = group.symbol;

  return true;
}

bool is_packet_readLSB(klee::ref<klee::Expr> expr, bytes_t &offset, int &n_bytes) {
  ASSERT(!expr.isNull(), "Null expr");

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
  ASSERT(group.has_symbol, "Group has no symbol");

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
  ASSERT(!expr.isNull(), "Null expr");

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

  ASSERT(size <= 64, "Size too big");

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
    ASSERT(width <= 64, "Width too big");
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

bool manager_contains(const klee::ConstraintManager &constraints,
                      klee::ref<klee::Expr> expr) {
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
  ASSERT(!obj_addr.isNull(), "Null obj_addr");
  ASSERT(is_constant(obj_addr), "Non-constant obj_addr");
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
    ASSERT(false, "TODO");
  }

  return constraint;
}
} // namespace synapse