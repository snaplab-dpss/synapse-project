#include "exprs.h"
#include "solver.h"
#include "../log.h"

namespace synapse {
solver_toolbox_t solver_toolbox;

solver_toolbox_t::solver_toolbox_t()
    : solver(createCexCachingSolver(klee::createCoreSolver(klee::Z3_SOLVER))),
      exprBuilder(klee::createDefaultExprBuilder()) {
  assert(solver && "Failed to create solver");
  assert(exprBuilder && "Failed to create exprBuilder");
}

bool solver_toolbox_t::is_expr_always_true(klee::ref<klee::Expr> expr) const {
  static std::unordered_map<unsigned, bool> cache;

  klee::ConstraintManager no_constraints;

  auto it = cache.find(expr->hash());
  if (it != cache.end()) {
    return it->second;
  }

  bool res = is_expr_always_true(no_constraints, expr);
  cache[expr->hash()] = res;

  return res;
}

bool solver_toolbox_t::is_expr_always_true(const klee::ConstraintManager &constraints,
                                           klee::ref<klee::Expr> expr) const {
  klee::Query sat_query(constraints, expr);

  bool result = false;
  bool success = solver->mustBeTrue(sat_query, result);
  assert(success && "Failed to check if expr is always true");

  return result;
}

bool solver_toolbox_t::is_expr_maybe_true(const klee::ConstraintManager &constraints,
                                          klee::ref<klee::Expr> expr) const {
  klee::Query sat_query(constraints, expr);

  bool result = false;
  bool success = solver->mayBeTrue(sat_query, result);
  assert(success && "Failed to check if expr is maybe true");

  return result;
}

bool solver_toolbox_t::is_expr_maybe_false(const klee::ConstraintManager &constraints,
                                           klee::ref<klee::Expr> expr) const {
  klee::Query sat_query(constraints, expr);

  bool result = false;
  bool success = solver->mayBeFalse(sat_query, result);
  assert(success && "Failed to check if expr is maybe false");

  return result;
}

bool solver_toolbox_t::are_exprs_always_equal(klee::ref<klee::Expr> e1, klee::ref<klee::Expr> e2,
                                              klee::ConstraintManager c1,
                                              klee::ConstraintManager c2) const {
  klee::ref<klee::Expr> eq_expr = exprBuilder->Eq(e1, e2);

  klee::Query eq_in_e1_ctx_sat_query(c1, eq_expr);
  klee::Query eq_in_e2_ctx_sat_query(c2, eq_expr);

  bool eq_in_e1_ctx = false;
  bool eq_in_e2_ctx = false;

  bool eq_in_e1_ctx_success = solver->mustBeTrue(eq_in_e1_ctx_sat_query, eq_in_e1_ctx);
  bool eq_in_e2_ctx_success = solver->mustBeTrue(eq_in_e2_ctx_sat_query, eq_in_e2_ctx);

  assert(eq_in_e1_ctx_success && "Failed to check if exprs are always equal");
  assert(eq_in_e2_ctx_success && "Failed to check if exprs are always equal");

  return eq_in_e1_ctx && eq_in_e2_ctx;
}

bool solver_toolbox_t::are_exprs_always_not_equal(klee::ref<klee::Expr> e1,
                                                  klee::ref<klee::Expr> e2,
                                                  klee::ConstraintManager c1,
                                                  klee::ConstraintManager c2) const {
  klee::ref<klee::Expr> eq_expr = exprBuilder->Eq(e1, e2);

  klee::Query eq_in_e1_ctx_sat_query(c1, eq_expr);
  klee::Query eq_in_e2_ctx_sat_query(c2, eq_expr);

  bool not_eq_in_e1_ctx = false;
  bool not_eq_in_e2_ctx = false;

  bool not_eq_in_e1_ctx_success = solver->mustBeFalse(eq_in_e1_ctx_sat_query, not_eq_in_e1_ctx);
  bool not_eq_in_e2_ctx_success = solver->mustBeFalse(eq_in_e2_ctx_sat_query, not_eq_in_e2_ctx);

  assert(not_eq_in_e1_ctx_success && "Failed to check if exprs are always equal");
  assert(not_eq_in_e2_ctx_success && "Failed to check if exprs are always equal");

  return not_eq_in_e1_ctx && not_eq_in_e2_ctx;
}

bool solver_toolbox_t::is_expr_always_false(klee::ref<klee::Expr> expr) const {
  static std::unordered_map<unsigned, bool> cache;

  klee::ConstraintManager no_constraints;

  auto it = cache.find(expr->hash());
  if (it != cache.end()) {
    return it->second;
  }

  bool res = is_expr_always_false(no_constraints, expr);
  cache[expr->hash()] = res;

  return res;
}

bool solver_toolbox_t::is_expr_always_false(const klee::ConstraintManager &constraints,
                                            klee::ref<klee::Expr> expr) const {
  klee::Query sat_query(constraints, expr);

  bool result = false;
  bool success = solver->mustBeFalse(sat_query, result);
  assert(success && "Failed to check if expr is always false");

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

  klee::ref<klee::Expr> eq = exprBuilder->Eq(expr1, expr2);

  return is_expr_always_true(eq);
}

u64 solver_toolbox_t::value_from_expr(klee::ref<klee::Expr> expr) const {
  assert(expr->getWidth() <= 64 && "Width too big");

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
  assert(success && "Failed to get value from expr");

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
  assert(success && "Failed to get value from expr");

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

    const arg_t &arg1 = arg.second;
    const arg_t &arg2 = found->second;

    klee::ref<klee::Expr> expr1 = arg1.expr;
    klee::ref<klee::Expr> expr2 = arg2.expr;

    klee::ref<klee::Expr> in1 = arg1.in;
    klee::ref<klee::Expr> in2 = arg2.in;

    klee::ref<klee::Expr> out1 = arg1.out;
    klee::ref<klee::Expr> out2 = arg2.out;

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
  klee::Expr::Width width = expr->getWidth();
  u64 value = solver_toolbox.value_from_expr(expr, constraints);

  u64 mask = 0;
  for (u64 i = 0u; i < width; i++) {
    mask <<= 1;
    mask |= 1;
  }

  return -((~value + 1) & mask);
}
} // namespace synapse