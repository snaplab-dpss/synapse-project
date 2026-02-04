#pragma once

#include <LibCore/Types.h>

namespace LibSynapse {
namespace GlobalStats {

extern u64 num_phase1_speculations;
extern u64 num_phase2_speculations;

extern u64 num_speculated_modules;
extern u64 num_execution_plans_generated;

extern time_us_t total_time_spent_speculating;
extern time_us_t total_time_spent_generating_execution_plans;

inline time_us_t time_per_speculation() { return num_speculated_modules == 0 ? 0 : total_time_spent_speculating / num_speculated_modules; }

inline time_us_t time_per_instantiation() {
  return num_execution_plans_generated == 0 ? 0 : total_time_spent_generating_execution_plans / num_execution_plans_generated;
}

} // namespace GlobalStats
} // namespace LibSynapse