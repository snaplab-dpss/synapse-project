#include <LibBDD/CallPathsGroups.h>
#include <LibCore/Solver.h>
#include <LibCore/Debug.h>
#include <LibCore/Expr.h>

namespace LibBDD {

using LibCore::solver_toolbox;

void CallPathsGroup::group_call_paths() {
  assert(call_paths.data.size() && "No call paths to group");

  for (call_path_t *cp : call_paths.data) {
    on_true.data.clear();
    on_false.data.clear();

    if (cp->calls.empty()) {
      continue;
    }

    const call_t &call = cp->calls[0];

    for (call_path_t *other_cp : call_paths.data) {
      if (other_cp->calls.size() && are_calls_equal(other_cp->calls[0], call)) {
        on_true.data.push_back(other_cp);
        continue;
      }

      on_false.data.push_back(other_cp);
    }

    // All calls are equal, no need do discriminate
    if (on_false.data.empty()) {
      return;
    }

    constraint = find_discriminating_constraint();

    if (!constraint.isNull()) {
      return;
    }
  }

  if (on_true.data.empty() && on_false.data.empty()) {
    on_true = call_paths;
    return;
  }

  // Last shot...
  for (size_t i = 0; i < call_paths.data.size(); i++) {
    on_true.data.clear();
    on_false.data.clear();

    for (size_t j = 0; j < call_paths.data.size(); j++) {
      call_path_t *call_path = call_paths.data[j];

      if (i == j) {
        on_true.data.push_back(call_path);
      } else {
        on_false.data.push_back(call_path);
      }
    }

    constraint = find_discriminating_constraint();

    if (!constraint.isNull()) {
      return;
    }
  }

  panic("Could not group call paths");
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

    if (!c1_arg.out.isNull() && !solver_toolbox.are_exprs_always_equal(c1_arg.in, c1_arg.out))
      continue;

    // comparison between modifications to the received packet
    if (!c1_arg.in.isNull() && !solver_toolbox.are_exprs_always_equal(c1_arg.in, c2_arg.in))
      return false;

    if (c1_arg.in.isNull() && !solver_toolbox.are_exprs_always_equal(c1_arg.expr, c2_arg.expr))
      return false;
  }

  return true;
}

klee::ref<klee::Expr> CallPathsGroup::find_discriminating_constraint() {
  assert(on_true.data.size() && "No call paths on true");

  std::vector<klee::ref<klee::Expr>> possible_discriminating_constraints = get_possible_discriminating_constraints();

  for (klee::ref<klee::Expr> candidate : possible_discriminating_constraints) {
    if (check_discriminating_constraint(candidate))
      return candidate;
  }

  return klee::ref<klee::Expr>();
}

std::vector<klee::ref<klee::Expr>> CallPathsGroup::get_possible_discriminating_constraints() const {
  std::vector<klee::ref<klee::Expr>> possible_discriminating_constraints;
  assert(on_true.data.size() && "No call paths on true");

  for (klee::ref<klee::Expr> candidate : on_true.data[0]->constraints) {
    if (satisfies_constraint(on_true.data, candidate))
      possible_discriminating_constraints.push_back(candidate);
  }

  return possible_discriminating_constraints;
}

bool CallPathsGroup::satisfies_constraint(std::vector<call_path_t *> cps, klee::ref<klee::Expr> target_constraint) const {
  for (call_path_t *call_path : cps) {
    if (!satisfies_constraint(call_path, target_constraint))
      return false;
  }
  return true;
}

bool CallPathsGroup::satisfies_constraint(call_path_t *call_path, klee::ref<klee::Expr> target_constraint) const {
  klee::ref<klee::Expr> not_constraint = solver_toolbox.exprBuilder->Not(target_constraint);
  return solver_toolbox.is_expr_always_false(call_path->constraints, not_constraint);
}

bool CallPathsGroup::satisfies_not_constraint(std::vector<call_path_t *> cps, klee::ref<klee::Expr> target_constraint) const {
  for (call_path_t *call_path : cps) {
    if (!satisfies_not_constraint(call_path, target_constraint))
      return false;
  }
  return true;
}

bool CallPathsGroup::satisfies_not_constraint(call_path_t *call_path, klee::ref<klee::Expr> target_constraint) const {
  klee::ref<klee::Expr> not_constraint = solver_toolbox.exprBuilder->Not(target_constraint);
  return solver_toolbox.is_expr_always_true(call_path->constraints, not_constraint);
}

bool CallPathsGroup::check_discriminating_constraint(klee::ref<klee::Expr> target_constraint) {
  assert(on_true.data.size() && "No call paths on true");
  assert(on_false.data.size() && "No call paths on false");

  call_paths_view_t _on_true = on_true;
  call_paths_view_t _on_false;

  for (call_path_t *call_path : on_false.data) {
    if (satisfies_constraint(call_path, target_constraint)) {
      _on_true.data.push_back(call_path);
    } else {
      _on_false.data.push_back(call_path);
    }
  }

  if (_on_false.data.size() && satisfies_not_constraint(_on_false.data, target_constraint)) {
    on_true  = _on_true;
    on_false = _on_false;
    return true;
  }

  return false;
}

std::vector<klee::ref<klee::Expr>> CallPathsGroup::get_common_constraints() const {
  std::vector<klee::ref<klee::Expr>> common_constraints;

  for (klee::ref<klee::Expr> common_constraint : on_true.data[0]->constraints) {
    if (satisfies_constraint(on_true.data, common_constraint) && satisfies_constraint(on_false.data, common_constraint)) {
      common_constraints.push_back(common_constraint);
    }
  }

  return common_constraints;
}

} // namespace LibBDD