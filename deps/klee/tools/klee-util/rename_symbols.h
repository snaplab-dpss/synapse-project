#pragma once

#include "expr/Parser.h"
#include "klee/Constraints.h"
#include "klee/ExprBuilder.h"
#include "klee/util/ExprVisitor.h"

#include "solver_toolbox.h"

namespace kutil {

class RenameSymbols : public klee::ExprVisitor::ExprVisitor {
private:
  std::map<std::string, std::string> translations;

public:
  RenameSymbols() {}
  RenameSymbols(const RenameSymbols &renamer)
      : klee::ExprVisitor::ExprVisitor(true),
        translations(renamer.translations) {}

  const std::map<std::string, std::string> &get_translations() const {
    return translations;
  }

  void add_translation(std::string before, std::string after) {
    translations[before] = after;
  }

  void remove_translation(std::string before) { translations.erase(before); }

  bool has_translation(std::string before) const {
    auto found_it = translations.find(before);
    return found_it != translations.end();
  }

  klee::ref<klee::Expr> rename(klee::ref<klee::Expr> expr) {
    if (expr.isNull()) {
      return expr;
    }

    return visit(expr);
  }

  klee::ConstraintManager rename(klee::ConstraintManager &constraints) {
    klee::ConstraintManager renamed_constraints;

    for (auto constraint : constraints) {
      auto renamed_constraint = rename(constraint);
      renamed_constraints.addConstraint(renamed_constraint);
    }

    return renamed_constraints;
  }

  klee::ExprVisitor::Action visitRead(const klee::ReadExpr &e) {
    auto ul = e.updates;
    auto root = ul.root;
    auto symbol = root->getName();
    auto found_it = translations.find(symbol);

    if (found_it != translations.end()) {
      auto replaced = klee::expr::ExprHandle(const_cast<klee::ReadExpr *>(&e));
      auto new_root = solver_toolbox.arr_cache.CreateArray(
          found_it->second, root->getSize(),
          root->constantValues.begin().base(),
          root->constantValues.end().base(), root->getDomain(),
          root->getRange());

      auto new_ul = klee::UpdateList(new_root, ul.head);
      auto replacement = solver_toolbox.exprBuilder->Read(new_ul, e.index);

      return Action::changeTo(replacement);
    }

    return Action::doChildren();
  }
};

} // namespace kutil