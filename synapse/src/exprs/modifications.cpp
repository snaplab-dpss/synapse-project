#include <klee/util/ExprVisitor.h>

#include "exprs.h"
#include "solver.h"

#include "../log.h"

std::vector<mod_t> build_expr_mods(klee::ref<klee::Expr> before,
                                   klee::ref<klee::Expr> after) {
  assert(before->getWidth() == after->getWidth());

  if (after->getKind() == klee::Expr::Concat) {
    bits_t msb_width = after->getKid(0)->getWidth();
    bits_t lsb_width = after->getWidth() - msb_width;

    std::vector<mod_t> msb_groups = build_expr_mods(
        solver_toolbox.exprBuilder->Extract(before, lsb_width, msb_width),
        after->getKid(0));

    std::vector<mod_t> lsb_groups = build_expr_mods(
        solver_toolbox.exprBuilder->Extract(before, 0, lsb_width),
        after->getKid(1));

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