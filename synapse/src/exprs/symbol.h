#pragma once

#include <unordered_set>
#include <optional>
#include <unordered_set>

#include <klee/Expr.h>

#include "../types.h"

namespace synapse {

struct symbol_t {
  std::string base;
  const klee::Array *array;
  klee::ref<klee::Expr> expr;
};

struct symbol_hash_t {
  std::size_t operator()(const symbol_t &s) const noexcept;
};

struct symbol_equal_t {
  bool operator()(const symbol_t &a, const symbol_t &b) const noexcept;
};

typedef std::unordered_set<symbol_t, symbol_hash_t, symbol_equal_t> symbols_t;

symbol_t build_symbol(const klee::Array *array);
symbol_t create_symbol(const std::string &label, bits_t size);

bool get_symbol(const symbols_t &symbols, const std::string &base, symbol_t &symbol);

} // namespace synapse