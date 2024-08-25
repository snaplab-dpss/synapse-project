#include <unordered_map>
#include <map>

#include <klee/util/ExprVisitor.h>

#include "replacer.h"
#include "solver.h"

class SymbolReplacer : public klee::ExprVisitor::ExprVisitor {
private:
  std::unordered_map<std::string, klee::UpdateList> roots_updates;
  std::map<klee::ref<klee::Expr>, klee::ref<klee::Expr>> replacements;

public:
  SymbolReplacer(const std::vector<klee::ref<klee::ReadExpr>> &reads) {
    for (klee::ref<klee::ReadExpr> read : reads) {
      const klee::UpdateList &ul = read->updates;
      const klee::Array *root = ul.root;
      roots_updates.insert({root->name, ul});
    }
  }

  klee::ExprVisitor::Action visitExprPost(const klee::Expr &e) {
    std::map<klee::ref<klee::Expr>, klee::ref<klee::Expr>>::const_iterator it =
        replacements.find(klee::ref<klee::Expr>(const_cast<klee::Expr *>(&e)));

    if (it != replacements.end()) {
      return Action::changeTo(it->second);
    } else {
      return Action::doChildren();
    }
  }

  klee::ExprVisitor::Action visitRead(const klee::ReadExpr &e) {
    klee::ref<klee::Expr> replaced = const_cast<klee::ReadExpr *>(&e);

    klee::UpdateList ul = e.updates;
    const klee::Array *root = ul.root;

    if (roots_updates.find(root->name) != roots_updates.end()) {
      const klee::UpdateList &new_ul = roots_updates.at(root->name);
      klee::ref<klee::Expr> new_read =
          solver_toolbox.exprBuilder->Read(new_ul, e.index);
      auto it = replacements.find(new_read);

      if (it == replacements.end()) {
        replacements.insert({replaced, new_read});
      }

      return Action::changeTo(new_read);
    }

    return Action::doChildren();
  }
};

klee::ref<klee::Expr>
replace_symbols(klee::ref<klee::Expr> expr,
                const std::vector<klee::ref<klee::ReadExpr>> &reads) {
  SymbolReplacer rs(reads);
  return rs.visit(expr);
}