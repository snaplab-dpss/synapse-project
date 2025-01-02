#include <klee/util/ExprVisitor.h>

#include "renamer.h"
#include "solver.h"

namespace synapse {
class SymbolRenamer : public klee::ExprVisitor::ExprVisitor {
private:
  // Before -> After
  std::unordered_map<std::string, std::string> translations;

public:
  SymbolRenamer(const std::unordered_map<std::string, std::string> &_translations)
      : translations(_translations) {}

  klee::ref<klee::Expr> rename(klee::ref<klee::Expr> expr) {
    if (expr.isNull())
      return expr;
    return visit(expr);
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
    const std::string &symbol = root->getName();
    auto found_it = translations.find(symbol);

    if (found_it != translations.end()) {
      klee::ref<klee::Expr> replaced(const_cast<klee::ReadExpr *>(&e));
      const klee::Array *new_root = solver_toolbox.arr_cache.CreateArray(
          found_it->second, root->getSize(), root->constantValues.begin().base(),
          root->constantValues.end().base(), root->getDomain(), root->getRange());

      klee::UpdateList new_ul(new_root, ul.head);
      klee::ref<klee::Expr> replacement =
          solver_toolbox.exprBuilder->Read(new_ul, e.index);

      return Action::changeTo(replacement);
    }

    return Action::doChildren();
  }
};

klee::ref<klee::Expr>
rename_symbols(klee::ref<klee::Expr> expr,
               const std::unordered_map<std::string, std::string> &translations) {
  SymbolRenamer renamer(translations);
  return renamer.rename(expr);
}

klee::ConstraintManager
rename_symbols(const klee::ConstraintManager &constraints,
               const std::unordered_map<std::string, std::string> &translations) {
  SymbolRenamer renamer(translations);
  return renamer.rename(constraints);
}
} // namespace synapse