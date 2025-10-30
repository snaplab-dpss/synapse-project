#include <LibCore/SymbolManager.h>
#include <LibCore/Solver.h>

#include <klee/util/ExprVisitor.h>
#include <klee/Constraints.h>

#include <iostream>

namespace LibCore {

namespace {
class ArrayChecker : public klee::ExprVisitor::ExprVisitor {
private:
  const std::unordered_map<std::string, const klee::Array *> &names;
  bool found_unknown_array;

public:
  ArrayChecker(const SymbolManager *manager) : names(manager->get_names()), found_unknown_array(false) {}

  Action visitRead(const klee::ReadExpr &e) override final {
    klee::UpdateList updates = e.updates;
    const klee::Array *array = updates.root;

    auto names_it   = names.find(updates.root->name);
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
  SymbolRenamer(SymbolManager *manager, const std::unordered_map<std::string, std::string> &_translations) : klee::ExprVisitor::ExprVisitor(true) {
    for (const auto &[old_name, new_name] : _translations) {
      if (!manager->has_symbol(new_name)) {
        const symbol_t &old = manager->get_symbol(old_name);
        manager->create_symbol(new_name, old.expr->getWidth());
      }
      translations.insert({old_name, manager->get_array(new_name)});
    }
  }

  klee::ref<klee::Expr> rename(klee::ref<klee::Expr> expr) { return expr.isNull() ? expr : visit(expr); }

  klee::ConstraintManager rename(const klee::ConstraintManager &constraints) {
    klee::ConstraintManager renamed_constraints;
    for (klee::ref<klee::Expr> constraint : constraints) {
      klee::ref<klee::Expr> renamed_constraint = rename(constraint);
      renamed_constraints.addConstraint(renamed_constraint);
    }
    return renamed_constraints;
  }

  Action visitRead(const klee::ReadExpr &e) {
    klee::UpdateList ul       = e.updates;
    const klee::Array *root   = ul.root;
    auto found_translation_it = translations.find(root->name);

    if (found_translation_it != translations.end()) {
      klee::UpdateList new_ul(found_translation_it->second, ul.head);
      klee::ref<klee::Expr> replacement = solver_toolbox.exprBuilder->Read(new_ul, e.index);
      return Action::changeTo(replacement);
    }

    return Action::doChildren();
  }
};

} // namespace

symbol_t SymbolManager::store_clone(const klee::Array *array) {
  auto symbols_it = symbols.find(array->name);

  if (symbols_it == symbols.end()) {
    const bits_t size = array->size * 8;
    return create_symbol(array->name, size);
  }

  return symbols_it->second;
}

const std::vector<const klee::Array *> &SymbolManager::get_arrays() const { return arrays; }

const std::unordered_map<std::string, const klee::Array *> &SymbolManager::get_names() const { return names; }

const klee::Array *SymbolManager::get_array(const std::string &name) const {
  auto names_it = names.find(name);
  assert(names_it != names.end() && "Array not found");
  return names_it->second;
}

bool SymbolManager::has_symbol(const std::string &name) const { return symbols.find(name) != symbols.end(); }

void SymbolManager::remove_symbol(const std::string &name) {
  auto symbols_it = symbols.find(name);
  if (symbols_it != symbols.end()) {
    symbols.erase(symbols_it);
  }
  auto names_it = names.find(name);
  if (names_it != names.end()) {
    arrays.erase(std::remove(arrays.begin(), arrays.end(), names_it->second), arrays.end());
    names.erase(names_it);
  }
}

symbol_t SymbolManager::get_symbol(const std::string &name) const {
  auto symbols_it = symbols.find(name);
  assert(symbols_it != symbols.end() && "Symbol not found");
  return symbols_it->second;
}

Symbols SymbolManager::get_symbols() const {
  Symbols result;
  for (const auto &symbol : symbols) {
    result.add(symbol.second);
  }
  return result;
}

Symbols SymbolManager::get_symbols_with_base(const std::string &base) const { return get_symbols().filter_by_base(base); }

symbol_t SymbolManager::create_symbol(const std::string &name, bits_t size) {
  assert(!name.empty() && "Empty name");
  auto symbols_it = symbols.find(name);

  if (symbols_it != symbols.end()) {
    const symbol_t &symbol = symbols_it->second;
    assert(symbol.expr->getWidth() == size && "Size mismatch");
    return symbol;
  }

  const klee::Expr::Width domain = klee::Expr::Int32;
  const klee::Expr::Width range  = klee::Expr::Int8;
  const klee::Array *array       = cache.CreateArray(name, size / 8, nullptr, nullptr, domain, range);

  const klee::UpdateList updates(array, nullptr);
  const bytes_t width = array->size;

  klee::ref<klee::Expr> expr;
  for (bits_t i = 0; i < width; i++) {
    klee::ref<klee::Expr> index = solver_toolbox.exprBuilder->Constant(i, array->domain);
    if (expr.isNull()) {
      expr = solver_toolbox.exprBuilder->Read(updates, index);
      continue;
    }
    expr = solver_toolbox.exprBuilder->Concat(solver_toolbox.exprBuilder->Read(updates, index), expr);
  }

  const std::string base = symbol_t::base_from_name(name);
  const symbol_t symbol(base, name, expr);

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

klee::ref<klee::Expr> SymbolManager::translate(klee::ref<klee::Expr> expr, std::unordered_map<std::string, std::string> translations) {
  if (expr.isNull()) {
    return expr;
  }

  SymbolRenamer renamer(this, translations);
  return renamer.rename(expr);
}

void SymbolManager::dbg() const {
  std::cerr << "======== SymbolManager ========\n";
  for (const auto &symbol : symbols) {
    const symbol_t &s = symbol.second;
    std::cerr << s << "\n";
  }
  std::cerr << "===============================\n";
}

} // namespace LibCore