#include <LibSynapse/GlobalStats.h>

namespace LibSynapse {
namespace GlobalStats {

u64 num_phase1_speculations = 0;
u64 num_phase2_speculations = 0;
u64 num_phase3_speculations = 0;

u64 num_speculated_modules        = 0;
u64 num_execution_plans_generated = 0;

} // namespace GlobalStats
} // namespace LibSynapse