#pragma once

#include <LibCore/Types.h>

#include <memory>

#include <klee/Expr/ExprBuilder.h>
#include <klee/Solver/Solver.h>

namespace LibCore {

struct solver_toolbox_t {
  std::unique_ptr<klee::Solver> solver;
  std::unique_ptr<klee::ExprBuilder> exprBuilder;

  solver_toolbox_t();

  bool is_expr_always_true(klee::ref<klee::Expr> expr) const;
  bool is_expr_always_true(const klee::ConstraintSet &constraints, klee::ref<klee::Expr> expr) const;

  bool is_expr_always_false(klee::ref<klee::Expr> expr) const;
  bool is_expr_always_false(const klee::ConstraintSet &constraints, klee::ref<klee::Expr> expr) const;

  bool is_expr_maybe_true(const klee::ConstraintSet &constraints, klee::ref<klee::Expr> expr) const;
  bool is_expr_maybe_false(const klee::ConstraintSet &constraints, klee::ref<klee::Expr> expr) const;

  bool are_exprs_always_equal(klee::ref<klee::Expr> e1, klee::ref<klee::Expr> e2, klee::ConstraintSet c1, klee::ConstraintSet c2) const;

  bool are_exprs_always_not_equal(klee::ref<klee::Expr> e1, klee::ref<klee::Expr> e2, klee::ConstraintSet c1, klee::ConstraintSet c2) const;

  bool are_exprs_always_equal(klee::ref<klee::Expr> expr1, klee::ref<klee::Expr> expr2) const;

  bool strict_value_from_expr(klee::ref<klee::Expr> expr, u64 &value) const;
  u64 value_from_expr(klee::ref<klee::Expr> expr) const;
  u64 value_from_expr(klee::ref<klee::Expr> expr, const klee::ConstraintSet &constraints) const;
  i64 signed_value_from_expr(klee::ref<klee::Expr> expr, const klee::ConstraintSet &constraints) const;
};

extern solver_toolbox_t solver_toolbox;

} // namespace LibCore