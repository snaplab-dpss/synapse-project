#pragma once

#include <vector>
#include <unordered_map>

#include <klee/Expr.h>
#include <klee/util/ArrayCache.h>

namespace synapse {

class ArrayManager {
private:
  std::vector<const klee::Array *> arrays;
  std::unordered_map<std::string, const klee::Array *> names;
  klee::ArrayCache cache;

public:
  ArrayManager() {}

  const std::vector<const klee::Array *> &get_arrays() const;
  const std::unordered_map<std::string, const klee::Array *> &get_names() const;

  const klee::Array *save_clone(const klee::Array *array);
  bool manages_all_in_expr(klee::ref<klee::Expr> expr) const;
};

} // namespace synapse