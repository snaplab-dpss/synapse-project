#include <LibSynapse/GlobalStats.h>

namespace LibSynapse {
namespace GlobalStats {

u64 num_phase1_speculations = 0;
u64 num_phase2_speculations = 0;

u64 num_speculated_modules        = 0;
u64 num_execution_plans_generated = 0;

time_us_t total_time_spent_speculating;
time_us_t total_time_spent_generating_execution_plans;

} // namespace GlobalStats
} // namespace LibSynapse