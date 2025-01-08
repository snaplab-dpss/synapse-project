#pragma once

#include "symbol_manager.h"

#include <vector>
#include <memory>

#include <klee/ExprBuilder.h>
#include <klee/Expr.h>

namespace synapse {

struct kQuery_t {
  Symbols symbols;
  std::vector<klee::ref<klee::Expr>> values;
  std::vector<klee::ref<klee::Expr>> constraints;

  std::string dump(const SymbolManager *manager) const;
};

class kQueryParser {
private:
  SymbolManager *manager;
  std::unique_ptr<klee::ExprBuilder> builder;

public:
  kQueryParser(SymbolManager *_manager);

  kQuery_t parse(const std::string &kQueryStr);
  klee::ref<klee::Expr> parse_expr(const std::string &expr_str);
};

} // namespace synapse