#include "symbol_manager.h"
#include "../system.h"
#include "../util/solver.h"
#include "../util/exprs.h"

#include <klee/ExprBuilder.h>
#include <klee/util/ExprVisitor.h>
#include <klee/Constraints.h>

namespace synapse {

namespace {
class ArrayChecker : public klee::ExprVisitor::ExprVisitor {
private:
  const std::unordered_map<std::string, const klee::Array *> &names;
  bool found_unknown_array;

public:
  ArrayChecker(const SymbolManager *manager)
      : names(manager->get_names()), found_unknown_array(false) {}

  klee::ExprVisitor::Action visitRead(const klee::ReadExpr &e) override final {
    klee::UpdateList updates = e.updates;
    const klee::Array *array = updates.root;

    auto names_it = names.find(updates.root->name);
    bool is_unknown = (names_it == names.end() || names_it->second != array);
    found_unknown_array &= is_unknown;

    if (is_unknown) {
      return Action::skipChildren();
    }

    return Action::doChildren();
  }

  bool has_unknown_arrays() const { return found_unknown_array; }
};

class SymbolRenamer : public klee::ExprVisitor::ExprVisitor {
private:
  std::unordered_map<std::string, const klee::Array *> translations;

public:
  SymbolRenamer(SymbolManager *manager,
                const std::unordered_map<std::string, std::string> &_translations)
      : klee::ExprVisitor::ExprVisitor(true) {
    for (const auto &[old_name, new_name] : _translations) {
      const symbol_t &old = manager->get_symbol(old_name);
      bits_t size = old.expr->getWidth();
      manager->create_symbol(new_name, size);
      translations.insert({old_name, manager->get_array(new_name)});
    }
  }

  klee::ref<klee::Expr> rename(klee::ref<klee::Expr> expr) {
    return expr.isNull() ? expr : visit(expr);
  }

  klee::ConstraintManager rename(const klee::ConstraintManager &constraints) {
    klee::ConstraintManager renamed_constraints;
    for (klee::ref<klee::Expr> constraint : constraints) {
      klee::ref<klee::Expr> renamed_constraint = rename(constraint);
      renamed_constraints.addConstraint(renamed_constraint);
    }
    return renamed_constraints;
  }

  klee::ExprVisitor::Action visitRead(const klee::ReadExpr &e) {
    klee::UpdateList ul = e.updates;
    const klee::Array *root = ul.root;
    auto found_translation_it = translations.find(root->name);

    if (found_translation_it != translations.end()) {
      klee::UpdateList new_ul(found_translation_it->second, ul.head);
      klee::ref<klee::Expr> replacement = solver_toolbox.exprBuilder->Read(new_ul, e.index);
      return Action::changeTo(replacement);
    }

    return Action::doChildren();
  }
};

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

void SymbolManager::store_clone(const klee::Array *array) {
  if (names.find(array->name) == names.end()) {
    bits_t size = array->size * 8;
    create_symbol(array->name, size);
  }
}

const std::vector<const klee::Array *> &SymbolManager::get_arrays() const { return arrays; }

const std::unordered_map<std::string, const klee::Array *> &SymbolManager::get_names() const {
  return names;
}

const klee::Array *SymbolManager::get_array(const std::string &name) const {
  auto names_it = names.find(name);
  SYNAPSE_ASSERT(names_it != names.end(), "Array \"%s\" not found", name.c_str());
  return names_it->second;
}

bool SymbolManager::has_symbol(const std::string &name) const {
  return symbols.find(name) != symbols.end();
}

symbol_t SymbolManager::get_symbol(const std::string &name) const {
  auto symbols_it = symbols.find(name);
  SYNAPSE_ASSERT(symbols_it != symbols.end(), "Symbol \"%s\" not found", name.c_str());
  return symbols_it->second;
}

symbols_t SymbolManager::get_symbols() const {
  symbols_t result;
  for (const auto &symbol : symbols) {
    result.insert(symbol.second);
  }
  return result;
}

symbols_t SymbolManager::get_symbols_with_base(const std::string &base) const {
  symbols_t result;
  for (const auto &symbol : symbols) {
    if (symbol.second.base == base) {
      result.insert(symbol.second);
    }
  }
  return result;
}

symbol_t SymbolManager::create_symbol(const std::string &name, bits_t size) {
  SYNAPSE_ASSERT(!name.empty(), "Empty name");
  auto symbols_it = symbols.find(name);

  if (symbols_it != symbols.end()) {
    const symbol_t &symbol = symbols_it->second;
    SYNAPSE_ASSERT(symbol.expr->getWidth() == size,
                   "Size mismatch (expr=%s, size=%u, requested=%u)",
                   expr_to_string(symbol.expr, true).c_str(), symbol.expr->getWidth(), size);
    return symbol;
  }

  klee::Expr::Width domain = klee::Expr::Int32;
  klee::Expr::Width range = klee::Expr::Int8;
  const klee::Array *array = cache.CreateArray(name, size / 8, nullptr, nullptr, domain, range);

  std::unique_ptr<klee::ExprBuilder> builder =
      std::unique_ptr<klee::ExprBuilder>(klee::createDefaultExprBuilder());
  klee::UpdateList updates(array, nullptr);
  bytes_t width = array->size;

  klee::ref<klee::Expr> expr;
  for (size_t i = 0; i < width; i++) {
    klee::ref<klee::Expr> index = builder->Constant(i, array->domain);
    if (expr.isNull()) {
      expr = builder->Read(updates, index);
      continue;
    }
    expr = builder->Concat(builder->Read(updates, index), expr);
  }

  std::string base = base_from_name(name);
  symbol_t symbol(base, name, expr);

  arrays.push_back(array);
  names.insert({name, array});
  symbols.insert({name, symbol});

  return symbol;
}

bool SymbolManager::manages(klee::ref<klee::Expr> expr) const {
  if (expr.isNull()) {
    return true;
  }

  ArrayChecker array_checker(this);
  array_checker.visit(expr);

  return !array_checker.has_unknown_arrays();
}

klee::ref<klee::Expr>
SymbolManager::translate(klee::ref<klee::Expr> expr,
                         std::unordered_map<std::string, std::string> translations) {
  SymbolRenamer renamer(this, translations);
  return renamer.rename(expr);
}

} // namespace synapse