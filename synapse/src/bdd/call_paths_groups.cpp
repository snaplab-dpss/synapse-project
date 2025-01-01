#include "call_paths_groups.h"
#include "../exprs/exprs.h"
#include "../exprs/solver.h"
#include "../log.h"

void CallPathsGroup::group_call_paths() {
  ASSERT(call_paths.size(), "No call paths to group");

  for (call_path_t *cp : call_paths) {
    on_true.clear();
    on_false.clear();

    if (cp->calls.size() == 0)
      continue;

    const call_t &call = cp->calls[0];

    for (call_path_t *cp : call_paths) {
      if (cp->calls.size() && are_calls_equal(cp->calls[0], call)) {
        on_true.push_back(cp);
        continue;
      }

      on_false.push_back(cp);
    }

    // all calls are equal, no need do discriminate
    if (on_false.size() == 0)
      return;

    constraint = find_discriminating_constraint();

    if (!constraint.isNull())
      return;
  }

  // no more calls
  if (on_true.size() == 0 && on_false.size() == 0) {
    on_true = call_paths;
    return;
  }

  // Last shot...
  for (unsigned i = 0; i < call_paths.size(); i++) {
    on_true.clear();
    on_false.clear();

    for (unsigned j = 0; j < call_paths.size(); j++) {
      call_path_t *call_path = call_paths[j];

      if (i == j) {
        on_true.push_back(call_path);
      } else {
        on_false.push_back(call_path);
      }
    }

    constraint = find_discriminating_constraint();

    if (!constraint.isNull())
      return;
  }

  ASSERT(false, "Could not group call paths");
}

bool CallPathsGroup::are_calls_equal(call_t c1, call_t c2) {
  if (c1.function_name != c2.function_name)
    return false;

  for (auto arg_name_value_pair : c1.args) {
    const std::string &arg_name = arg_name_value_pair.first;

    // exception: we don't care about 'p' differences
    if (arg_name == "p" || arg_name == "src_devices")
      continue;

    const arg_t &c1_arg = c1.args[arg_name];
    const arg_t &c2_arg = c2.args[arg_name];

    if (!c1_arg.out.isNull() &&
        !solver_toolbox.are_exprs_always_equal(c1_arg.in, c1_arg.out))
      continue;

    // comparison between modifications to the received packet
    if (!c1_arg.in.isNull() &&
        !solver_toolbox.are_exprs_always_equal(c1_arg.in, c2_arg.in))
      return false;

    if (c1_arg.in.isNull() &&
        !solver_toolbox.are_exprs_always_equal(c1_arg.expr, c2_arg.expr))
      return false;
  }

  return true;
}

klee::ref<klee::Expr> CallPathsGroup::find_discriminating_constraint() {
  ASSERT(on_true.size(), "No call paths on true");

  auto possible_discriminating_constraints = get_possible_discriminating_constraints();

  for (auto constraint : possible_discriminating_constraints) {
    if (check_discriminating_constraint(constraint))
      return constraint;
  }

  return klee::ref<klee::Expr>();
}

std::vector<klee::ref<klee::Expr>>
CallPathsGroup::get_possible_discriminating_constraints() const {
  std::vector<klee::ref<klee::Expr>> possible_discriminating_constraints;
  ASSERT(on_true.size(), "No call paths on true");

  for (auto constraint : on_true[0]->constraints) {
    if (satisfies_constraint(on_true, constraint))
      possible_discriminating_constraints.push_back(constraint);
  }

  return possible_discriminating_constraints;
}

bool CallPathsGroup::satisfies_constraint(std::vector<call_path_t *> call_paths,
                                          klee::ref<klee::Expr> constraint) const {
  for (const auto &call_path : call_paths) {
    if (!satisfies_constraint(call_path, constraint))
      return false;
  }
  return true;
}

bool CallPathsGroup::satisfies_constraint(call_path_t *call_path,
                                          klee::ref<klee::Expr> constraint) const {
  auto not_constraint = solver_toolbox.exprBuilder->Not(constraint);
  return solver_toolbox.is_expr_always_false(call_path->constraints, not_constraint);
}

bool CallPathsGroup::satisfies_not_constraint(std::vector<call_path_t *> call_paths,
                                              klee::ref<klee::Expr> constraint) const {
  for (const auto &call_path : call_paths) {
    if (!satisfies_not_constraint(call_path, constraint))
      return false;
  }
  return true;
}

bool CallPathsGroup::satisfies_not_constraint(call_path_t *call_path,
                                              klee::ref<klee::Expr> constraint) const {
  auto not_constraint = solver_toolbox.exprBuilder->Not(constraint);
  return solver_toolbox.is_expr_always_true(call_path->constraints, not_constraint);
}

bool CallPathsGroup::check_discriminating_constraint(klee::ref<klee::Expr> constraint) {
  ASSERT(on_true.size(), "No call paths on true");
  ASSERT(on_false.size(), "No call paths on false");

  call_paths_view_t _on_true = on_true;
  call_paths_view_t _on_false;

  for (call_path_t *call_path : on_false) {
    if (satisfies_constraint(call_path, constraint)) {
      _on_true.push_back(call_path);
    } else {
      _on_false.push_back(call_path);
    }
  }

  if (_on_false.size() && satisfies_not_constraint(_on_false, constraint)) {
    on_true = _on_true;
    on_false = _on_false;
    return true;
  }

  return false;
}