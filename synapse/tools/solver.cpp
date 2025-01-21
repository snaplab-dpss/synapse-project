#include <LibCore/Solver.h>
#include <LibCore/SymbolManager.h>
#include <LibCore/Expr.h>

#include <iostream>

int main() {
  std::cout << "============ Test 1 ============\n";

  LibCore::SymbolManager symbol_manager;
  LibCore::symbol_t A1 = symbol_manager.create_symbol("A", 32);
  std::cout << "A1 " << LibCore::expr_to_string(A1.expr) << "\n";

  LibCore::symbol_t A2 = symbol_manager.create_symbol("A", 32);
  std::cout << "A2 " << LibCore::expr_to_string(A2.expr) << "\n";

  klee::ref<klee::Expr> eq = LibCore::solver_toolbox.exprBuilder->Eq(A1.expr, A2.expr);

  klee::ConstraintManager constraints;
  klee::Query sat_query(constraints, eq);

  bool result;
  bool success = LibCore::solver_toolbox.solver->mustBeTrue(sat_query, result);
  assert(success);

  std::cout << "Equal " << result << "\n";
  std::cout << "\n";

  klee::ref<klee::Expr> expr1 = LibCore::solver_toolbox.exprBuilder->Concat(
      LibCore::solver_toolbox.exprBuilder->Add(A1.expr, LibCore::solver_toolbox.exprBuilder->Constant(1, 32)),
      LibCore::solver_toolbox.exprBuilder->Add(symbol_manager.create_symbol("B", 32).expr,
                                               LibCore::solver_toolbox.exprBuilder->Constant(1, 32)));

  klee::ref<klee::Expr> expr2 = LibCore::solver_toolbox.exprBuilder->Concat(
      LibCore::solver_toolbox.exprBuilder->Add(A1.expr, LibCore::solver_toolbox.exprBuilder->Constant(1, 32)),
      LibCore::solver_toolbox.exprBuilder->Add(symbol_manager.create_symbol("B", 32).expr,
                                               LibCore::solver_toolbox.exprBuilder->Constant(12, 32)));

  std::cout << "expr1 " << LibCore::expr_to_string(expr1) << "\n";
  std::cout << "expr2 " << LibCore::expr_to_string(expr2) << "\n";

  for (const LibCore::expr_mod_t &g : LibCore::build_expr_mods(expr1, expr2)) {
    std::cout << "offset: " << g.offset << "\n";
    std::cout << "width:  " << g.width << "\n";
    std::cout << "expr:   " << LibCore::expr_to_string(g.expr) << "\n";
    std::cout << "\n";
  }

  std::cout << "\n";

  LibCore::symbol_t A = symbol_manager.create_symbol("B", 32);
  LibCore::symbol_t B = symbol_manager.create_symbol("B", 32);

  const klee::Array *A_array = symbol_manager.get_array("A");
  const klee::Array *B_array = symbol_manager.get_array("B");

  klee::ref<klee::Expr> zero  = LibCore::solver_toolbox.exprBuilder->Constant(0, 32);
  klee::ref<klee::Expr> one   = LibCore::solver_toolbox.exprBuilder->Constant(1, 32);
  klee::ref<klee::Expr> two   = LibCore::solver_toolbox.exprBuilder->Constant(2, 32);
  klee::ref<klee::Expr> three = LibCore::solver_toolbox.exprBuilder->Constant(3, 32);

  klee::ref<klee::Expr> zbyte = LibCore::solver_toolbox.exprBuilder->Constant(0, 8);

  klee::UpdateList a0_ul(A_array, nullptr);
  klee::UpdateList a1_ul(A_array, nullptr);
  klee::UpdateList a2_ul(A_array, nullptr);
  klee::UpdateList a3_ul(A_array, nullptr);

  klee::UpdateList b0_ul(B_array, nullptr);
  klee::UpdateList b1_ul(B_array, nullptr);
  klee::UpdateList b2_ul(B_array, nullptr);
  klee::UpdateList b3_ul(B_array, nullptr);

  klee::ref<klee::Expr> a0 = LibCore::solver_toolbox.exprBuilder->Read(a0_ul, zero);
  klee::ref<klee::Expr> a1 = LibCore::solver_toolbox.exprBuilder->Read(a1_ul, one);
  klee::ref<klee::Expr> a2 = LibCore::solver_toolbox.exprBuilder->Read(a2_ul, two);
  klee::ref<klee::Expr> a3 = LibCore::solver_toolbox.exprBuilder->Read(a3_ul, three);

  klee::ref<klee::Expr> b0 = LibCore::solver_toolbox.exprBuilder->Read(b0_ul, zero);
  klee::ref<klee::Expr> b1 = LibCore::solver_toolbox.exprBuilder->Read(b1_ul, one);
  klee::ref<klee::Expr> b2 = LibCore::solver_toolbox.exprBuilder->Read(b2_ul, two);
  klee::ref<klee::Expr> b3 = LibCore::solver_toolbox.exprBuilder->Read(b3_ul, three);

  klee::ref<klee::Expr> e0 = LibCore::solver_toolbox.exprBuilder->Concat(
      a0, LibCore::solver_toolbox.exprBuilder->Concat(
              b1, LibCore::solver_toolbox.exprBuilder->Concat(zbyte, LibCore::solver_toolbox.exprBuilder->Concat(a2, b3))));
  klee::ref<klee::Expr> e1 = LibCore::solver_toolbox.exprBuilder->Concat(
      b3, LibCore::solver_toolbox.exprBuilder->Concat(
              a2, LibCore::solver_toolbox.exprBuilder->Concat(zbyte, LibCore::solver_toolbox.exprBuilder->Concat(b1, a0))));

  for (const LibCore::expr_mod_t &g : LibCore::build_expr_mods(e0, e1)) {
    std::cout << "offset: " << g.offset << "\n";
    std::cout << "width:  " << g.width << "\n";
    std::cout << "expr:   " << LibCore::expr_to_string(g.expr) << "\n";
    std::cout << "\n";
  }

  std::cout << "\n";
  for (const LibCore::expr_byte_swap_t &byte_swap : LibCore::get_expr_byte_swaps(e0, e1)) {
    std::cout << "offset0: " << byte_swap.byte0 << "\n";
    std::cout << "offset1: " << byte_swap.byte1 << "\n";
    std::cout << "\n";
  }

  std::cout << "\n";
  klee::ref<klee::Expr> true_expr  = LibCore::solver_toolbox.exprBuilder->True();
  klee::ref<klee::Expr> false_expr = LibCore::solver_toolbox.exprBuilder->False();

  std::cout << "True:  " << LibCore::expr_to_string(true_expr) << "\n";
  std::cout << "Conditional? " << LibCore::is_conditional(true_expr) << "\n";
  std::cout << "False: " << LibCore::expr_to_string(false_expr) << "\n";
  std::cout << "Conditional? " << LibCore::is_conditional(false_expr) << "\n";

  return 0;
}
