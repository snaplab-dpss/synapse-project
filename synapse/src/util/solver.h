#pragma once

#include <memory>

#include <klee/ExprBuilder.h>
#include <klee/Solver.h>

#include "../call_paths.h"

namespace synapse {
struct solver_toolbox_t {
  std::unique_ptr<klee::Solver> solver;
  std::unique_ptr<klee::ExprBuilder> exprBuilder;

  solver_toolbox_t();

  bool is_expr_always_true(klee::ref<klee::Expr> expr) const;
  bool is_expr_always_true(const klee::ConstraintManager &constraints,
                           klee::ref<klee::Expr> expr) const;

  bool is_expr_always_false(klee::ref<klee::Expr> expr) const;
  bool is_expr_always_false(const klee::ConstraintManager &constraints,
                            klee::ref<klee::Expr> expr) const;

  bool is_expr_maybe_true(const klee::ConstraintManager &constraints,
                          klee::ref<klee::Expr> expr) const;
  bool is_expr_maybe_false(const klee::ConstraintManager &constraints,
                           klee::ref<klee::Expr> expr) const;

  bool are_exprs_always_equal(klee::ref<klee::Expr> e1, klee::ref<klee::Expr> e2,
                              klee::ConstraintManager c1, klee::ConstraintManager c2) const;

  bool are_exprs_always_not_equal(klee::ref<klee::Expr> e1, klee::ref<klee::Expr> e2,
                                  klee::ConstraintManager c1, klee::ConstraintManager c2) const;

  bool are_exprs_always_equal(klee::ref<klee::Expr> expr1, klee::ref<klee::Expr> expr2) const;

  u64 value_from_expr(klee::ref<klee::Expr> expr) const;
  u64 value_from_expr(klee::ref<klee::Expr> expr, const klee::ConstraintManager &constraints) const;
  i64 signed_value_from_expr(klee::ref<klee::Expr> expr,
                             const klee::ConstraintManager &constraints) const;

  bool are_calls_equal(call_t c1, call_t c2) const;
};

extern solver_toolbox_t solver_toolbox;
} // namespace synapse