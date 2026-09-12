#pragma once

#include <LibCore/Types.h>

#include <map>
#include <string>

namespace LibSynapse {
namespace GlobalStats {

extern u64 num_phase1_speculations;
extern u64 num_phase2_speculations;

extern u64 num_speculated_modules;
extern u64 num_execution_plans_generated;
extern u64 num_context_copies;
extern time_us_t total_time_phase1;
extern time_us_t total_time_phase2;
// Hot BDD nodes (>= 0.1% of traffic) a top-level speculation pass sent to the controller.
extern std::map<u64, u64> hot_nodes_speculated_to_controller;
// Time spent in each module factory's speculate(), by factory name.
extern std::map<std::string, time_us_t> time_speculating_per_factory;
extern u64 num_profiler_cache_misses;
extern time_us_t time_spec_targets;
extern time_us_t time_search_implement;
extern time_us_t time_reorder_ops;
extern time_us_t time_reorder_eps;
extern u64 num_reorder_candidates;
extern u64 num_reorder_ops;
extern time_us_t time_search_ss_add;
extern time_us_t time_search_log;
extern time_us_t time_search_heuristic_add;
extern time_us_t time_search_pop;
extern time_us_t time_spec_affected;
extern time_us_t time_spec_append;
extern time_us_t time_spec_remove;
extern u64 num_profiler_copies;
extern u64 num_profiler_cache_clears;
// Speculated compute steps: appended to an action of their run, placed in a new action, placed
// after a speculated recirculation, or declined at the recirculation cap.
extern u64 num_spec_compute_appended;
extern u64 num_spec_compute_new_action;
extern u64 num_spec_compute_recirculated;
extern u64 num_spec_compute_cap_declined;
// Compute ops that reused an action placed on another path instead of placing their own
// (TofinoContext, compute_reuse_state_t), by either route, in the search and in speculation alike.
extern u64 num_compute_ops_reused;
// The subset of num_compute_ops_reused that got there by a shape match: the same function of
// operands equal but for one plain value, unified by naming or by a move before the op matched.
// An op that matched exactly at the first lookup is counted in num_compute_ops_reused only.
extern u64 num_compute_ops_shape_shared;

extern time_us_t total_time_spent_speculating;
extern time_us_t total_time_spent_generating_execution_plans;

inline time_us_t time_per_speculation() { return num_speculated_modules == 0 ? 0 : total_time_spent_speculating / num_speculated_modules; }

inline time_us_t time_per_instantiation() {
  return num_execution_plans_generated == 0 ? 0 : total_time_spent_generating_execution_plans / num_execution_plans_generated;
}

} // namespace GlobalStats
} // namespace LibSynapse