#include <LibSynapse/Heuristics/HeuristicConfig.h>

namespace LibSynapse {

heuristic_metadata_t HeuristicCfg::build_meta_tput_estimate(const EP *ep) {
  const Context &ctx         = ep->get_ctx();
  const Profiler &profiler   = ctx.get_profiler();
  const bytes_t avg_pkt_size = profiler.get_avg_pkt_bytes();
  const pps_t estimate_pps   = ep->estimate_tput_pps();
  const bps_t estimate_bps   = LibCore::pps2bps(estimate_pps, avg_pkt_size);

  std::stringstream ss;
  ss << LibCore::tput2str(estimate_bps, "bps", true);

  ss << " (";
  ss << LibCore::tput2str(estimate_pps, "pps", true);
  ss << ")";

  const heuristic_metadata_t meta{
      .name        = "Tput",
      .description = ss.str(),
  };

  return meta;
}

heuristic_metadata_t HeuristicCfg::build_meta_tput_speculation(const EP *ep) {
  const Context &ctx          = ep->get_ctx();
  const Profiler &profiler    = ctx.get_profiler();
  const bytes_t avg_pkt_size  = profiler.get_avg_pkt_bytes();
  const pps_t speculation_pps = ep->speculate_tput_pps();
  const bps_t speculation_bps = LibCore::pps2bps(speculation_pps, avg_pkt_size);

  std::stringstream ss;
  ss << LibCore::tput2str(speculation_bps, "bps", true);

  ss << " (";
  ss << LibCore::tput2str(speculation_pps, "pps", true);
  ss << ")";

  const heuristic_metadata_t meta{
      .name        = "Spec",
      .description = ss.str(),
  };

  return meta;
}

} // namespace LibSynapse