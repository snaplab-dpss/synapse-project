#include <LibSynapse/GlobalStats.h>

namespace LibSynapse {
namespace GlobalStats {

u64 num_phase1_speculations = 0;
u64 num_phase2_speculations = 0;

u64 num_speculated_modules        = 0;
u64 num_context_copies            = 0;
time_us_t total_time_phase1       = 0;
time_us_t total_time_phase2       = 0;
std::map<u64, u64> hot_nodes_speculated_to_controller;
u64 num_spec_compute_appended     = 0;
u64 num_spec_compute_new_action   = 0;
u64 num_spec_compute_recirculated = 0;
u64 num_spec_compute_cap_declined = 0;
u64 num_execution_plans_generated = 0;

time_us_t total_time_spent_speculating;
time_us_t total_time_spent_generating_execution_plans;

} // namespace GlobalStats
} // namespace LibSynapse