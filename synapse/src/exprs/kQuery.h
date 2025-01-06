#pragma once

#include "array_manager.h"

#include <vector>
#include <memory>

#include <klee/ExprBuilder.h>
#include <klee/Expr.h>

namespace synapse {

struct kQuery_t {
  std::vector<klee::ref<klee::Expr>> values;
  std::vector<klee::ref<klee::Expr>> constraints;

  std::string dump(const ArrayManager *manager) const;
};

class kQueryParser {
private:
  ArrayManager *manager;
  std::unique_ptr<klee::ExprBuilder> builder;

public:
  kQueryParser(ArrayManager *_manager);

  kQuery_t parse(const std::string &kQueryStr);
  klee::ref<klee::Expr> parse_expr(const std::string &expr_str);
};

} // namespace synapse