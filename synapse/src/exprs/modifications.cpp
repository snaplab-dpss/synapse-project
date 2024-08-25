#include "exprs.h"
#include "solver.h"

std::vector<modification_t> build_modifications(klee::ref<klee::Expr> before,
                                                klee::ref<klee::Expr> after) {
  std::vector<modification_t> _modifications;

  assert(before->getWidth() == after->getWidth());
  klee::Expr::Width size = before->getWidth();

  for (size_t b = 0; b < size; b += 8) {
    klee::ref<klee::Expr> before_byte =
        solver_toolbox.exprBuilder->Extract(before, b, klee::Expr::Int8);
    klee::ref<klee::Expr> after_byte =
        solver_toolbox.exprBuilder->Extract(after, b, klee::Expr::Int8);

    bool eq = solver_toolbox.are_exprs_always_equal(before_byte, after_byte);
    if (eq) {
      continue;
    }

    _modifications.emplace_back(b / 8, after_byte);
  }

  return _modifications;
}