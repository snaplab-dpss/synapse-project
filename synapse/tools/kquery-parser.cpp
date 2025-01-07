#include <iostream>
#include <assert.h>
#include <vector>

#include "../src/exprs/solver.h"
#include "../src/exprs/symbol_manager.h"
#include "../src/exprs/kQuery.h"

using synapse::kQuery_t;
using synapse::kQueryParser;
using synapse::SymbolManager;

kQuery_t parse(const std::string &kQueryStr, SymbolManager *manager) {
  kQueryParser kQueryParser(manager);
  kQuery_t kQuery = kQueryParser.parse(kQueryStr);
  return kQuery;
}

int main() {
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
    std::cerr << "->" << array->getName() << "[" << array->getSize() << "] : w"
              << array->getDomain() << " -> w" << array->getRange() << " = ";
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

  return 0;
}