#include "symbol.h"

namespace BDD {
bool has_label(const symbols_t &symbols, const std::string &base) {
  auto found_it = std::find_if(
      symbols.begin(), symbols.end(),
      [&](const BDD::symbol_t &symbol) { return symbol.label_base == base; });

  return found_it != symbols.end();
}

std::string get_label(const symbols_t &symbols, const std::string &base) {
  auto found_it = std::find_if(
      symbols.begin(), symbols.end(),
      [&](const BDD::symbol_t &symbol) { return symbol.label_base == base; });

  assert(found_it != symbols.end());
  return found_it->label;
}

BDD::symbol_t get_symbol(const symbols_t &symbols, const std::string &base) {
  auto found_it = std::find_if(
      symbols.begin(), symbols.end(),
      [&](const BDD::symbol_t &symbol) { return symbol.label_base == base; });

  assert(found_it != symbols.end());
  return *found_it;
}

} // namespace BDD