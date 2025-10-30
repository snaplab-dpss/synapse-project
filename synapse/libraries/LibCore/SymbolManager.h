#pragma once

#include <LibCore/Types.h>
#include <LibCore/Symbol.h>

#include <vector>
#include <unordered_map>

#include <klee/Expr.h>
#include <klee/util/ArrayCache.h>

namespace LibCore {

class SymbolManager {
private:
  std::vector<const klee::Array *> arrays;
  std::unordered_map<std::string, const klee::Array *> names;
  std::unordered_map<std::string, symbol_t> symbols;
  klee::ArrayCache cache;

public:
  SymbolManager()                                      = default;
  SymbolManager(const SymbolManager &other)            = delete;
  SymbolManager(SymbolManager &&other)                 = default;
  SymbolManager &operator=(const SymbolManager &other) = delete;

  const std::vector<const klee::Array *> &get_arrays() const;
  const std::unordered_map<std::string, const klee::Array *> &get_names() const;
  const klee::Array *get_array(const std::string &name) const;

  bool has_symbol(const std::string &name) const;
  symbol_t get_symbol(const std::string &name) const;
  Symbols get_symbols() const;
  Symbols get_symbols_with_base(const std::string &base) const;
  bool manages(klee::ref<klee::Expr> expr) const;
  void remove_symbol(const std::string &name);
  void dbg() const;

  symbol_t store_clone(const klee::Array *array);
  symbol_t create_symbol(const std::string &name, bits_t size);
  klee::ref<klee::Expr> translate(klee::ref<klee::Expr> expr, std::unordered_map<std::string, std::string> translations);
};

} // namespace LibCore