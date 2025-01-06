#include "symbol.h"
#include "solver.h"
#include "../system.h"

namespace synapse {
namespace {
std::string base_from_name(const std::string &name) {
  SYNAPSE_ASSERT(name.size(), "Empty name");

  if (!std::isdigit(name.back())) {
    return name;
  }

  size_t delim = name.rfind("_");
  SYNAPSE_ASSERT(delim != std::string::npos, "Invalid name");

  std::string base = name.substr(0, delim);
  return base;
}
} // namespace

bool get_symbol(const symbols_t &symbols, const std::string &base, symbol_t &symbol) {
  for (const symbol_t &s : symbols) {
    if (s.base == base) {
      symbol = s;
      return true;
    }
  }
  return false;
}

symbol_t build_symbol(const klee::Array *array) {
  std::string base = base_from_name(array->name);
  klee::ref<klee::Expr> expr = solver_toolbox.create_new_symbol(array);
  return symbol_t({base, array, expr});
}

symbol_t create_symbol(const std::string &label, bits_t size) {
  const klee::Array *array;

  klee::ref<klee::Expr> expr = solver_toolbox.create_new_symbol(label, size, array);

  symbol_t new_symbol = {
      .base = label,
      .array = array,
      .expr = expr,
  };

  return new_symbol;
}

std::size_t symbol_hash_t::operator()(const symbol_t &s) const noexcept {
  return s.array->hash();
}

bool symbol_equal_t::operator()(const symbol_t &a, const symbol_t &b) const noexcept {
  return solver_toolbox.are_exprs_always_equal(a.expr, b.expr);
}
} // namespace synapse