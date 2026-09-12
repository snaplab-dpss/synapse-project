#include <LibSynapse/GlobalStats.h>

namespace LibSynapse {
namespace GlobalStats {

u64 num_phase1_speculations = 0;
u64 num_phase2_speculations = 0;

u64 num_speculated_modules  = 0;
u64 num_context_copies      = 0;
time_us_t total_time_phase1 = 0;
time_us_t total_time_phase2 = 0;
std::map<u64, u64> hot_nodes_speculated_to_controller;
std::map<std::string, time_us_t> time_speculating_per_factory;
u64 num_profiler_cache_misses       = 0;
time_us_t time_spec_targets         = 0;
time_us_t time_search_implement     = 0;
time_us_t time_reorder_ops          = 0;
time_us_t time_reorder_eps          = 0;
u64 num_reorder_candidates          = 0;
u64 num_reorder_ops                 = 0;
time_us_t time_search_ss_add        = 0;
time_us_t time_search_log           = 0;
time_us_t time_search_heuristic_add = 0;
time_us_t time_search_pop           = 0;
time_us_t time_spec_affected        = 0;
time_us_t time_spec_append          = 0;
time_us_t time_spec_remove          = 0;
u64 num_profiler_copies             = 0;
u64 num_profiler_cache_clears       = 0;
u64 num_spec_compute_appended       = 0;
u64 num_spec_compute_new_action     = 0;
u64 num_spec_compute_recirculated   = 0;
u64 num_spec_compute_cap_declined   = 0;
u64 num_compute_ops_reused          = 0;
u64 num_execution_plans_generated   = 0;

time_us_t total_time_spent_speculating;
time_us_t total_time_spent_generating_execution_plans;

} // namespace GlobalStats
} // namespace LibSynapse