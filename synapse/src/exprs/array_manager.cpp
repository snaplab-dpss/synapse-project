#include "array_manager.h"

#include <klee/util/ExprVisitor.h>

namespace synapse {

namespace {
class ArrayChecker : public klee::ExprVisitor::ExprVisitor {
private:
  const std::unordered_map<std::string, const klee::Array *> &names;
  bool found_unknown_array;

public:
  ArrayChecker(const ArrayManager *manager)
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

} // namespace

const klee::Array *ArrayManager::save_clone(const klee::Array *array) {
  if (names.find(array->name) != names.end()) {
    return array;
  }

  const klee::Array *new_array = cache.CreateArray(array->name, array->size, nullptr,
                                                   nullptr, array->domain, array->range);
  arrays.push_back(new_array);
  names.insert({array->name, new_array});

  return new_array;
}

const std::vector<const klee::Array *> &ArrayManager::get_arrays() const {
  return arrays;
}

const std::unordered_map<std::string, const klee::Array *> &
ArrayManager::get_names() const {
  return names;
}

bool ArrayManager::manages_all_in_expr(klee::ref<klee::Expr> expr) const {
  if (expr.isNull()) {
    return true;
  }

  ArrayChecker array_checker(this);
  array_checker.visit(expr);

  return !array_checker.has_unknown_arrays();
}

} // namespace synapse