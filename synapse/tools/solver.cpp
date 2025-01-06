#include <iostream>

#include "../src/exprs/solver.h"
#include "../src/exprs/exprs.h"
#include "../src/exprs/retriever.h"

using namespace synapse;

int main() {
  klee::ref<klee::Expr> A1 = solver_toolbox.create_new_symbol("A", 64);
  std::cerr << "A1 " << expr_to_string(A1) << "\n";

  klee::ref<klee::Expr> A2 = solver_toolbox.create_new_symbol("A", 64);
  std::cerr << "A2 " << expr_to_string(A2) << "\n";

  klee::ref<klee::Expr> eq = solver_toolbox.exprBuilder->Eq(A1, A2);

  klee::ConstraintManager constraints;
  klee::Query sat_query(constraints, eq);

  bool result;
  bool success = solver_toolbox.solver->mustBeTrue(sat_query, result);
  assert(success);

  std::cerr << "Equal " << result << "\n";

  klee::ref<klee::Expr> expr1 = solver_toolbox.exprBuilder->Concat(
      solver_toolbox.exprBuilder->Add(A1, solver_toolbox.exprBuilder->Constant(1, 64)),
      solver_toolbox.exprBuilder->Add(solver_toolbox.create_new_symbol("B", 64),
                                      solver_toolbox.exprBuilder->Constant(1, 64)));

  klee::ref<klee::Expr> expr2 = solver_toolbox.exprBuilder->Concat(
      solver_toolbox.exprBuilder->Add(A1, solver_toolbox.exprBuilder->Constant(1, 64)),
      solver_toolbox.exprBuilder->Add(solver_toolbox.create_new_symbol("B", 64),
                                      solver_toolbox.exprBuilder->Constant(12, 64)));

  std::cerr << "expr1 " << expr_to_string(expr1) << "\n";
  std::cerr << "expr2 " << expr_to_string(expr2) << "\n";

  for (const mod_t &g : build_expr_mods(expr1, expr2)) {
    std::cerr << "offset: " << g.offset << "\n";
    std::cerr << "width:  " << g.width << "\n";
    std::cerr << "expr:   " << expr_to_string(g.expr) << "\n";
    std::cerr << "\n";
  }

  return 0;
}
