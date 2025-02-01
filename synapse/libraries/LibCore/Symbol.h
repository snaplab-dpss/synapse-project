#pragma once

#include <unordered_set>
#include <optional>

#include <klee/Expr.h>

namespace LibCore {

struct symbol_t {
  std::string base;
  std::string name;
  klee::ref<klee::Expr> expr;

  symbol_t()                            = default;
  symbol_t(const symbol_t &)            = default;
  symbol_t(symbol_t &&)                 = default;
  symbol_t &operator=(const symbol_t &) = default;
  ~symbol_t()                           = default;

  symbol_t(klee::ref<klee::Expr> expr);
  symbol_t(const std::string &base, const std::string &name, klee::ref<klee::Expr> expr);

  static bool is_symbol(klee::ref<klee::Expr> expr);
  static std::unordered_set<std::string> get_symbols_names(klee::ref<klee::Expr> expr);
};

std::ostream &operator<<(std::ostream &os, const symbol_t &symbol);

class Symbols {
private:
  struct symbol_hash_t {
    std::size_t operator()(const symbol_t &s) const noexcept;
  };

  struct symbol_equal_t {
    bool operator()(const symbol_t &a, const symbol_t &b) const noexcept;
  };

  std::unordered_set<symbol_t, symbol_hash_t, symbol_equal_t> data;

public:
  Symbols()                           = default;
  Symbols(const Symbols &)            = default;
  Symbols(Symbols &&)                 = default;
  Symbols &operator=(const Symbols &) = default;

  void add(const symbol_t &symbol);
  void add(const Symbols &symbols);

  size_t size() const;
  bool empty() const;
  std::vector<symbol_t> get() const;
  symbol_t get(const std::string &name) const;
  bool has(const std::string &name) const;
  bool remove(const std::string &name);
  Symbols filter_by_base(const std::string &base) const;

  friend std::ostream &operator<<(std::ostream &os, const Symbols &symbols);
};

std::ostream &operator<<(std::ostream &os, const Symbols &symbols);

} // namespace LibCore