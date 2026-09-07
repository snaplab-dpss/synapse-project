#pragma once

#include <LibCore/Types.h>

#include <map>

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
// Speculated compute steps: appended to an action of their run, placed in a new action, placed
// after a speculated recirculation, or declined at the recirculation cap.
extern u64 num_spec_compute_appended;
extern u64 num_spec_compute_new_action;
extern u64 num_spec_compute_recirculated;
extern u64 num_spec_compute_cap_declined;

extern time_us_t total_time_spent_speculating;
extern time_us_t total_time_spent_generating_execution_plans;

inline time_us_t time_per_speculation() { return num_speculated_modules == 0 ? 0 : total_time_spent_speculating / num_speculated_modules; }

inline time_us_t time_per_instantiation() {
  return num_execution_plans_generated == 0 ? 0 : total_time_spent_generating_execution_plans / num_execution_plans_generated;
}

} // namespace GlobalStats
} // namespace LibSynapse