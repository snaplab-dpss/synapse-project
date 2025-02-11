#pragma once

#include <LibBDD/CallPath.h>

namespace LibBDD {

class CallPathsGroup {
private:
  call_paths_view_t call_paths;
  klee::ref<klee::Expr> constraint;
  call_paths_view_t on_true;
  call_paths_view_t on_false;

private:
  void group_call_paths();
  bool check_discriminating_constraint(klee::ref<klee::Expr> constraint);
  klee::ref<klee::Expr> find_discriminating_constraint();
  std::vector<klee::ref<klee::Expr>> get_possible_discriminating_constraints() const;
  bool satisfies_constraint(std::vector<call_path_t *> call_paths, klee::ref<klee::Expr> constraint) const;
  bool satisfies_constraint(call_path_t *call_path, klee::ref<klee::Expr> constraint) const;
  bool satisfies_not_constraint(std::vector<call_path_t *> call_paths, klee::ref<klee::Expr> constraint) const;
  bool satisfies_not_constraint(call_path_t *call_path, klee::ref<klee::Expr> constraint) const;
  bool are_calls_equal(call_t c1, call_t c2);
  call_t pop_call();

public:
  CallPathsGroup(const call_paths_view_t &_call_paths, bool init_mode = false) : call_paths(_call_paths) {
    group_call_paths();

    if (init_mode) {
      on_true.data.erase(std::remove_if(on_true.data.begin(), on_true.data.end(), [](call_path_t *cp) { return cp->calls.empty(); }),
                         on_true.data.end());
      on_false.data.erase(std::remove_if(on_false.data.begin(), on_false.data.end(), [](call_path_t *cp) { return cp->calls.empty(); }),
                          on_false.data.end());

      if (on_true.data.empty() || on_false.data.empty()) {
        call_paths_view_t non_empty = on_true.data.empty() ? on_false : on_true;
        call_paths_view_t empty     = on_true.data.empty() ? on_true : on_false;

        on_true    = non_empty;
        on_false   = empty;
        constraint = nullptr;
      }
    }
  }

  klee::ref<klee::Expr> get_discriminating_constraint() const { return constraint; }
  std::vector<klee::ref<klee::Expr>> get_common_constraints() const;

  const call_paths_view_t &get_on_true() const { return on_true; }
  const call_paths_view_t &get_on_false() const { return on_false; }
};

} // namespace LibBDD