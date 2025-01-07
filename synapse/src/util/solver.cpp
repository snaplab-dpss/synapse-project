#include <iostream>

#include "exprs.h"
#include "solver.h"
#include "retriever.h"
#include "../log.h"

namespace synapse {
solver_toolbox_t solver_toolbox;

solver_toolbox_t::solver_toolbox_t()
    : solver(createCexCachingSolver(klee::createCoreSolver(klee::Z3_SOLVER))),
      exprBuilder(klee::createDefaultExprBuilder()) {
  SYNAPSE_ASSERT(solver, "Failed to create solver");
  SYNAPSE_ASSERT(exprBuilder, "Failed to create exprBuilder");
}

bool solver_toolbox_t::is_expr_always_true(klee::ref<klee::Expr> expr) const {
  klee::ConstraintManager no_constraints;

  struct expr_hash_t {
    std::size_t operator()(klee::ref<klee::Expr> expr) const { return expr->hash(); }
  };

  static std::unordered_map<klee::ref<klee::Expr>, bool, expr_hash_t> cache;
  auto it = cache.find(expr);
  if (it != cache.end()) {
    return it->second;
  }

  bool res = is_expr_always_true(no_constraints, expr);
  cache[expr] = res;

  return res;
}

bool solver_toolbox_t::is_expr_always_true(const klee::ConstraintManager &constraints,
                                           klee::ref<klee::Expr> expr) const {
  klee::Query sat_query(constraints, expr);

  bool result;
  bool success = solver->mustBeTrue(sat_query, result);
  SYNAPSE_ASSERT(success, "Failed to check if expr is always true");

  return result;
}

bool solver_toolbox_t::is_expr_maybe_true(const klee::ConstraintManager &constraints,
                                          klee::ref<klee::Expr> expr) const {
  klee::Query sat_query(constraints, expr);

  bool result;
  bool success = solver->mayBeTrue(sat_query, result);
  SYNAPSE_ASSERT(success, "Failed to check if expr is maybe true");

  return result;
}

bool solver_toolbox_t::is_expr_maybe_false(const klee::ConstraintManager &constraints,
                                           klee::ref<klee::Expr> expr) const {
  klee::Query sat_query(constraints, expr);

  bool result;
  bool success = solver->mayBeFalse(sat_query, result);
  SYNAPSE_ASSERT(success, "Failed to check if expr is maybe false");

  return result;
}

bool solver_toolbox_t::are_exprs_always_equal(klee::ref<klee::Expr> e1, klee::ref<klee::Expr> e2,
                                              klee::ConstraintManager c1,
                                              klee::ConstraintManager c2) const {
  auto eq_expr = exprBuilder->Eq(e1, e2);

  auto eq_in_e1_ctx_sat_query = klee::Query(c1, eq_expr);
  auto eq_in_e2_ctx_sat_query = klee::Query(c2, eq_expr);

  bool eq_in_e1_ctx;
  bool eq_in_e2_ctx;

  bool eq_in_e1_ctx_success = solver->mustBeTrue(eq_in_e1_ctx_sat_query, eq_in_e1_ctx);
  bool eq_in_e2_ctx_success = solver->mustBeTrue(eq_in_e2_ctx_sat_query, eq_in_e2_ctx);

  SYNAPSE_ASSERT(eq_in_e1_ctx_success, "Failed to check if exprs are always equal");
  SYNAPSE_ASSERT(eq_in_e2_ctx_success, "Failed to check if exprs are always equal");

  return eq_in_e1_ctx && eq_in_e2_ctx;
}

bool solver_toolbox_t::are_exprs_always_not_equal(klee::ref<klee::Expr> e1,
                                                  klee::ref<klee::Expr> e2,
                                                  klee::ConstraintManager c1,
                                                  klee::ConstraintManager c2) const {
  auto eq_expr = exprBuilder->Eq(e1, e2);

  auto eq_in_e1_ctx_sat_query = klee::Query(c1, eq_expr);
  auto eq_in_e2_ctx_sat_query = klee::Query(c2, eq_expr);

  bool not_eq_in_e1_ctx;
  bool not_eq_in_e2_ctx;

  bool not_eq_in_e1_ctx_success = solver->mustBeFalse(eq_in_e1_ctx_sat_query, not_eq_in_e1_ctx);
  bool not_eq_in_e2_ctx_success = solver->mustBeFalse(eq_in_e2_ctx_sat_query, not_eq_in_e2_ctx);

  SYNAPSE_ASSERT(not_eq_in_e1_ctx_success, "Failed to check if exprs are always equal");
  SYNAPSE_ASSERT(not_eq_in_e2_ctx_success, "Failed to check if exprs are always equal");

  return not_eq_in_e1_ctx && not_eq_in_e2_ctx;
}

bool solver_toolbox_t::is_expr_always_false(klee::ref<klee::Expr> expr) const {
  klee::ConstraintManager no_constraints;

  struct expr_hash_t {
    std::size_t operator()(klee::ref<klee::Expr> expr) const { return expr->hash(); }
  };

  static std::unordered_map<klee::ref<klee::Expr>, bool, expr_hash_t> cache;
  auto it = cache.find(expr);
  if (it != cache.end()) {
    return it->second;
  }

  bool res = is_expr_always_false(no_constraints, expr);
  cache[expr] = res;

  return res;
}

bool solver_toolbox_t::is_expr_always_false(const klee::ConstraintManager &constraints,
                                            klee::ref<klee::Expr> expr) const {
  klee::Query sat_query(constraints, expr);

  bool result;
  bool success = solver->mustBeFalse(sat_query, result);
  SYNAPSE_ASSERT(success, "Failed to check if expr is always false");

  return result;
}

bool solver_toolbox_t::are_exprs_always_equal(klee::ref<klee::Expr> expr1,
                                              klee::ref<klee::Expr> expr2) const {
  if (expr1.isNull() != expr2.isNull()) {
    return false;
  }

  if (expr1.isNull()) {
    return true;
  }

  if (expr1->getWidth() != expr2->getWidth()) {
    return false;
  }

  auto eq = exprBuilder->Eq(expr1, expr2);
  return is_expr_always_true(eq);
}

bool solver_toolbox_t::are_exprs_values_always_equal(klee::ref<klee::Expr> expr1,
                                                     klee::ref<klee::Expr> expr2) const {
  if (expr1.isNull() != expr2.isNull()) {
    return false;
  }

  if (expr1.isNull()) {
    return true;
  }

  auto v1 = value_from_expr(expr1);
  auto v2 = value_from_expr(expr2);

  auto v1_const = exprBuilder->Constant(v1, expr1->getWidth());
  auto v2_const = exprBuilder->Constant(v2, expr2->getWidth());

  auto always_v1 = are_exprs_always_equal(v1_const, expr1);
  auto always_v2 = are_exprs_always_equal(v2_const, expr2);

  if (!always_v1) {
    std::cerr << "are_exprs_values_always_equal error\n";
    std::cerr << "expr1 not always = " << expr_to_string(v1_const) << "\n";
    std::cerr << "expr1: " << expr_to_string(expr1) << "\n";
    SYNAPSE_ASSERT(false, "are_exprs_values_always_equal error");
    exit(1);
  }

  if (!always_v2) {
    std::cerr << "are_exprs_values_always_equal error\n";
    std::cerr << "expr2 not always = " << expr_to_string(v2_const) << "\n";
    std::cerr << "expr2: " << expr_to_string(expr2) << "\n";
    SYNAPSE_ASSERT(false, "are_exprs_values_always_equal error");
    exit(1);
  }

  return v1 == v2;
}

u64 solver_toolbox_t::value_from_expr(klee::ref<klee::Expr> expr) const {
  if (expr->getKind() == klee::Expr::Kind::Constant) {
    auto constant_expr = dynamic_cast<klee::ConstantExpr *>(expr.get());
    return constant_expr->getZExtValue();
  }

  struct expr_hash_t {
    std::size_t operator()(klee::ref<klee::Expr> expr) const { return expr->hash(); }
  };

  static std::unordered_map<klee::ref<klee::Expr>, u64, expr_hash_t> cache;
  auto it = cache.find(expr);
  if (it != cache.end()) {
    return it->second;
  }

  klee::ConstraintManager no_constraints;
  klee::Query sat_query(no_constraints, expr);

  klee::ref<klee::ConstantExpr> value_expr;
  bool success = solver->getValue(sat_query, value_expr);
  SYNAPSE_ASSERT(success, "Failed to get value from expr");

  u64 res = value_expr->getZExtValue();
  cache[expr] = res;

  return res;
}

u64 solver_toolbox_t::value_from_expr(klee::ref<klee::Expr> expr,
                                      const klee::ConstraintManager &constraints) const {
  if (expr->getKind() == klee::Expr::Kind::Constant) {
    auto constant_expr = dynamic_cast<klee::ConstantExpr *>(expr.get());
    return constant_expr->getZExtValue();
  }

  klee::Query sat_query(constraints, expr);

  klee::ref<klee::ConstantExpr> value_expr;
  bool success = solver->getValue(sat_query, value_expr);
  SYNAPSE_ASSERT(success, "Failed to get value from expr");

  return value_expr->getZExtValue();
}

bool solver_toolbox_t::are_calls_equal(call_t c1, call_t c2) const {
  if (c1.function_name != c2.function_name) {
    return false;
  }

  for (auto arg : c1.args) {
    auto found = c2.args.find(arg.first);
    if (found == c2.args.end()) {
      return false;
    }

    auto arg1 = arg.second;
    auto arg2 = found->second;

    auto expr1 = arg1.expr;
    auto expr2 = arg2.expr;

    auto in1 = arg1.in;
    auto in2 = arg2.in;

    auto out1 = arg1.out;
    auto out2 = arg2.out;

    if (expr1.isNull() != expr2.isNull()) {
      return false;
    }

    if (in1.isNull() != in2.isNull()) {
      return false;
    }

    if (out1.isNull() != out2.isNull()) {
      return false;
    }

    if (in1.isNull() && out1.isNull() && !are_exprs_always_equal(expr1, expr2)) {
      return false;
    }

    if (!in1.isNull() && !are_exprs_always_equal(in1, in2)) {
      return false;
    }
  }

  return true;
}

int64_t solver_toolbox_t::signed_value_from_expr(klee::ref<klee::Expr> expr,
                                                 const klee::ConstraintManager &constraints) const {
  auto width = expr->getWidth();
  auto value = solver_toolbox.value_from_expr(expr, constraints);

  u64 mask = 0;
  for (u64 i = 0u; i < width; i++) {
    mask <<= 1;
    mask |= 1;
  }

  return -((~value + 1) & mask);
}
} // namespace synapse