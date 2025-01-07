#include <iostream>

#include "../src/util/solver.h"
#include "../src/util/exprs.h"
#include "../src/util/retriever.h"
#include "../src/util/symbol_manager.h"

using synapse::build_expr_mods;
using synapse::expr_to_string;
using synapse::mod_t;
using synapse::solver_toolbox;
using synapse::symbol_t;
using synapse::SymbolManager;

int main() {
  SymbolManager symbol_manager;
  symbol_t A1 = symbol_manager.create_symbol("A", 64);
  std::cerr << "A1 " << expr_to_string(A1.expr) << "\n";

  symbol_t A2 = symbol_manager.create_symbol("A", 64);
  std::cerr << "A2 " << expr_to_string(A2.expr) << "\n";

  klee::ref<klee::Expr> eq = solver_toolbox.exprBuilder->Eq(A1.expr, A2.expr);

  klee::ConstraintManager constraints;
  klee::Query sat_query(constraints, eq);

  bool result;
  bool success = solver_toolbox.solver->mustBeTrue(sat_query, result);
  assert(success);

  std::cerr << "Equal " << result << "\n";

  klee::ref<klee::Expr> expr1 = solver_toolbox.exprBuilder->Concat(
      solver_toolbox.exprBuilder->Add(A1.expr, solver_toolbox.exprBuilder->Constant(1, 64)),
      solver_toolbox.exprBuilder->Add(symbol_manager.create_symbol("B", 64).expr,
                                      solver_toolbox.exprBuilder->Constant(1, 64)));

  klee::ref<klee::Expr> expr2 = solver_toolbox.exprBuilder->Concat(
      solver_toolbox.exprBuilder->Add(A1.expr, solver_toolbox.exprBuilder->Constant(1, 64)),
      solver_toolbox.exprBuilder->Add(symbol_manager.create_symbol("B", 64).expr,
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
