#pragma once

#include <LibCore/Types.h>

namespace LibSynapse {
namespace GlobalStats {

extern u64 num_phase1_speculations;
extern u64 num_phase2_speculations;
extern u64 num_phase3_speculations;

extern u64 num_speculated_modules;
extern u64 num_execution_plans_generated;

} // namespace GlobalStats
} // namespace LibSynapse