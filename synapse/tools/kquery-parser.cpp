#include <LibCore/kQuery.h>
#include <LibCore/Expr.h>
#include <LibCore/Solver.h>

#include <iostream>
#include <assert.h>
#include <vector>

using namespace LibCore;

kQuery_t parse(const std::string &kQueryStr, SymbolManager *manager) {
  kQueryParser kQueryParser(manager);
  kQuery_t kQuery = kQueryParser.parse(kQueryStr);
  return kQuery;
}

void test1() {
  const std::string query = "array loop_termination[4] : w32 -> w8 = symbolic\n"
                            "array starting_time[8] : w32 -> w8 = symbolic\n"
                            "(query [(Sle (w64 0) N0:(ReadLSB w64 (w32 0) starting_time)) (Eq "
                            "(w32 0) (ReadLSB w32 (w32 0) loop_termination))] false [N0])\n";

  SymbolManager manager;
  kQuery_t kQuery = parse(query, &manager);

  std::cerr << "\n========================================\n";
  std::cerr << "Query:\n";
  std::cerr << query << "\n";

  std::cerr << "Arrays:\n";
  for (const klee::Array *array : manager.get_arrays()) {
    std::cerr << "->" << array->getName() << "[" << array->getSize() << "] : w" << array->getDomain() << " -> w" << array->getRange() << " = ";
    if (array->isSymbolicArray()) {
      std::cerr << "symbolic\n";
    } else {
      std::cerr << "[" << array->getSize() << "]\n";
    }
  }

  std::cerr << "Values:\n";
  for (klee::ref<klee::Expr> expr : kQuery.values) {
    std::cerr << "->";
    expr->dump();
  }

  std::cerr << "Constraints:\n";
  for (klee::ref<klee::Expr> expr : kQuery.constraints) {
    std::cerr << "->";
    expr->dump();
  }

  std::cerr << "kQuery:\n";
  std::cerr << kQuery.dump(&manager) << "\n";

  std::cerr << "========================================\n";
}

// (Extract w16 0 (Or w32 (Shl w32 (And w32 (ZExt w32 (ReadLSB w16 (w32 514) packet_chunks)) (w32 255)) (w32 8)) (AShr w32 (And w32 (ZExt
// w32 (ReadLSB w16 (w32 514) packet_chunks)) (w32 65280)) (w32 8))))
void test2() {
  const std::string query =
      "array packet_chunks[1280] : w32 -> w8 = symbolic\n"
      "(query [] false [(Extract w16 0 (Or w32 (Shl w32 (And w32 (ZExt w32 (ReadLSB w16 (w32 514) packet_chunks)) (w32 255)) (w32 8)) "
      "(AShr w32 (And w32 (ZExt w32 (ReadLSB w16 (w32 514) packet_chunks)) (w32 65280)) (w32 8))))])\n";

  SymbolManager manager;
  kQuery_t kQuery = parse(query, &manager);

  std::cerr << "\n========================================\n";
  std::cerr << "Query:\n";
  std::cerr << query << "\n";

  assert(manager.get_arrays().size() == 1);
  assert(kQuery.values.size() == 1);
  assert(kQuery.constraints.size() == 0);

  klee::ref<klee::Expr> expr = kQuery.values[0];

  std::cerr << "Expr: " << expr_to_string(expr) << "\n";

  klee::ref<klee::Expr> target;
  bool is_endian_swap = match_endian_swap_pattern(expr, target);

  std::cerr << "Is endian swap: " << is_endian_swap << "\n";

  if (is_endian_swap) {
    std::cerr << "Target: " << expr_to_string(target) << "\n";
  }

  std::cerr << "========================================\n";
}

void test3() {
  const std::string query =
      "array packet_chunks[1280] : w32 -> w8 = symbolic\n"
      "(query [] false [(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 "
      "(w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 "
      "(Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) "
      "(Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))\n(Concat w104 (Read w8 (w32 265) "
      "packet_chunks) (Concat w96 (ReadLSB w64 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))])\n";

  SymbolManager manager;
  kQuery_t kQuery = parse(query, &manager);

  std::cerr << "\n========================================\n";
  std::cerr << "Query:\n";
  std::cerr << query << "\n";

  assert(manager.get_arrays().size() == 1);
  assert(kQuery.values.size() == 2);
  assert(kQuery.constraints.size() == 0);

  klee::ref<klee::Expr> expr0 = kQuery.values[0];
  klee::ref<klee::Expr> expr1 = kQuery.values[1];

  std::cerr << "Expr0: " << expr_to_string(expr0) << "\n";
  std::cerr << "Simple: " << expr_to_string(simplify(expr0)) << "\n";
  std::cerr << "\n";
  std::cerr << "Expr1: " << expr_to_string(expr1) << "\n";
  std::cerr << "Simple: " << expr_to_string(simplify(expr1)) << "\n";

  for (const auto &group : get_expr_groups(expr0)) {
    std::cerr << "\n";
    std::cerr << "has symbol? " << group.has_symbol << "\n";
    if (group.has_symbol) {
      std::cerr << "symbol: " << group.symbol << "\n";
    }
    std::cerr << "offset: " << group.offset << "\n";
    std::cerr << "size: " << group.size << "\n";
    std::cerr << "expr: " << expr_to_string(group.expr) << "\n";
  }

  std::cerr << "========================================\n";

  std::cerr << "eq? " << solver_toolbox.are_exprs_always_equal(expr0, expr1) << "\n";
}

void test4() {
  const std::string query =
      "array packet_chunks[1280] : w32 -> w8 = symbolic\n"
      "(query [] false [(ReadLSB w32 (w32 512) packet_chunks)\n(Concat w32 (Read w8 (w32 515) packet_chunks) (Concat w24 (Read w8 (w32 "
      "514) packet_chunks) (Concat w16 (Read w8 (w32 513) packet_chunks) (Read w8 (w32 512) packet_chunks))))])\n";

  SymbolManager manager;
  kQuery_t kQuery = parse(query, &manager);

  std::cerr << "\n========================================\n";
  std::cerr << "Query:\n";
  std::cerr << query << "\n";

  assert(manager.get_arrays().size() == 1);
  assert(kQuery.values.size() == 2);
  assert(kQuery.constraints.size() == 0);

  klee::ref<klee::Expr> expr0 = kQuery.values[0];
  klee::ref<klee::Expr> expr1 = kQuery.values[1];

  std::cerr << "Expr0: " << expr_to_string(expr0) << "\n";
  std::cerr << "Simple: " << expr_to_string(simplify(expr0)) << "\n";

  std::cerr << "\n";
  std::cerr << "Expr1: " << expr_to_string(expr1) << "\n";
  std::cerr << "Simple: " << expr_to_string(simplify(expr1)) << "\n";

  std::cerr << "========================================\n";
}

void test5() {
  const std::string query = "array vector_data_r35[1280] : w32 -> w8 = symbolic\n"
                            "(query [] false [(Extract w32 0 (Add w64 (w64 1) (SExt w64 (Extract w32 0 (Add w64 (w64 18446744073709551615) "
                            "(SExt w64 (ReadLSB w32 (w32 0) vector_data_r35)))))))])";

  SymbolManager manager;
  kQuery_t kQuery = parse(query, &manager);

  std::cerr << "\n========================================\n";
  std::cerr << "Query:\n";
  std::cerr << query << "\n";

  assert(manager.get_arrays().size() == 1);
  assert(kQuery.values.size() == 1);
  assert(kQuery.constraints.size() == 0);

  klee::ref<klee::Expr> expr      = kQuery.values[0];
  klee::ref<klee::Expr> explified = simplify(expr);

  std::cerr << "\n";
  std::cerr << "Expr:   " << expr_to_string(expr) << "\n";
  std::cerr << "Simple: " << expr_to_string(explified) << "\n";

  std::cerr << "========================================\n";
}

void test6() {
  const std::string query =
      "array packet_chunks[1280] : w32 -> w8 = symbolic\n"
      "(query [] false [(ReadLSB w200 (w32 112) packet_chunks)\n(ReadLSB w8 (w32 2048) packet_chunks)\n(ReadLSB w112 (w32 0) packet_chunks)])\n";

  SymbolManager manager;
  kQuery_t kQuery = parse(query, &manager);

  std::cerr << "\n========================================\n";
  std::cerr << "Query:\n";
  std::cerr << query << "\n";

  assert(manager.get_arrays().size() == 1);
  assert(kQuery.values.size() == 3);
  assert(kQuery.constraints.size() == 0);

  klee::ref<klee::Expr> expr0 = kQuery.values[0];
  klee::ref<klee::Expr> expr1 = kQuery.values[1];
  klee::ref<klee::Expr> expr2 = kQuery.values[2];

  std::cerr << "Expr0: " << expr_to_string(expr0) << "\n";
  std::cerr << "Expr1: " << expr_to_string(expr1) << "\n";
  std::cerr << "Expr2: " << expr_to_string(expr2) << "\n";

  Symbols symbols;
  symbols.add(symbol_t(expr0));
  symbols.add(symbol_t(expr1));
  symbols.add(symbol_t(expr2));

  std::cerr << "========================================\n";
}

int main() {
  // test1();
  // test2();
  // test3();
  // test4();
  // test5();
  test6();
  return 0;
}