#pragma once

#include <unordered_set>
#include <optional>
#include <unordered_set>

#include <klee/Expr.h>

#include "../types.h"

namespace synapse {

struct symbol_t {
  std::string base;
  std::string name;
  klee::ref<klee::Expr> expr;

  symbol_t() = default;
  symbol_t(const symbol_t &) = default;
  symbol_t(symbol_t &&) = default;
  symbol_t &operator=(const symbol_t &) = default;

  symbol_t(klee::ref<klee::Expr> expr);
  symbol_t(const std::string &base, const std::string &name, klee::ref<klee::Expr> expr);
};

struct symbol_hash_t {
  std::size_t operator()(const symbol_t &s) const noexcept;
};

struct symbol_equal_t {
  bool operator()(const symbol_t &a, const symbol_t &b) const noexcept;
};

typedef std::unordered_set<symbol_t, symbol_hash_t, symbol_equal_t> symbols_t;

std::ostream &operator<<(std::ostream &os, const symbol_t &symbol);
} // namespace synapse