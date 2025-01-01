#include <filesystem>
#include <fstream>
#include <CLI/CLI.hpp>
#include <toml++/toml.hpp>

#include "../src/bdd/bdd.h"
#include "../src/log.h"
#include "../src/visualizers/ep_visualizer.h"
#include "../src/visualizers/ss_visualizer.h"
#include "../src/visualizers/profiler_visualizer.h"
#include "../src/heuristics/heuristics.h"
#include "../src/search.h"
#include "../src/synthesizers/ep/synthesizers.h"
#include "../src/targets/targets.h"

enum class HeuristicOption {
  BFS,
  DFS,
  RANDOM,
  GALLIUM,
  GREEDY,
  MAX_TPUT,
  DS_PREF,
};

const std::unordered_map<std::string, HeuristicOption> heuristic_opt_converter{
    {"bfs", HeuristicOption::BFS},         {"dfs", HeuristicOption::DFS},
    {"random", HeuristicOption::RANDOM},   {"gallium", HeuristicOption::GALLIUM},
    {"greedy", HeuristicOption::GREEDY},   {"max-tput", HeuristicOption::MAX_TPUT},
    {"ds-pref", HeuristicOption::DS_PREF},
};

search_report_t search(const BDD *bdd, const toml::table &config,
                       const Profiler &profiler, HeuristicOption heuristic_opt,
                       const std::vector<ep_id_t> &peek, bool no_reorder,
                       bool pause_on_backtrack, bool not_greedy) {
  bool stop_on_first_solution = !not_greedy;

  std::unordered_set<ep_id_t> peek_set;
  for (ep_id_t ep_id : peek) {
    peek_set.insert(ep_id);
  }

  switch (heuristic_opt) {
  case HeuristicOption::BFS: {
    BFS heuristic(stop_on_first_solution);
    SearchEngine engine(bdd, &heuristic, config, profiler, !no_reorder, peek_set,
                        pause_on_backtrack);
    return engine.search();
  } break;
  case HeuristicOption::DFS: {
    DFS heuristic(stop_on_first_solution);
    SearchEngine engine(bdd, &heuristic, config, profiler, !no_reorder, peek_set,
                        pause_on_backtrack);
    return engine.search();
  } break;
  case HeuristicOption::RANDOM: {
    Random heuristic(stop_on_first_solution);
    SearchEngine engine(bdd, &heuristic, config, profiler, !no_reorder, peek_set,
                        pause_on_backtrack);
    return engine.search();
  } break;
  case HeuristicOption::GALLIUM: {
    Gallium heuristic(stop_on_first_solution);
    SearchEngine engine(bdd, &heuristic, config, profiler, !no_reorder, peek_set,
                        pause_on_backtrack);
    return engine.search();
  } break;
  case HeuristicOption::GREEDY: {
    Greedy heuristic(stop_on_first_solution);
    SearchEngine engine(bdd, &heuristic, config, profiler, !no_reorder, peek_set,
                        pause_on_backtrack);
    return engine.search();
  } break;
  case HeuristicOption::MAX_TPUT: {
    MaxTput heuristic(stop_on_first_solution);
    SearchEngine engine(bdd, &heuristic, config, profiler, !no_reorder, peek_set,
                        pause_on_backtrack);
    return engine.search();
  } break;
  case HeuristicOption::DS_PREF: {
    DSPref heuristic(stop_on_first_solution);
    SearchEngine engine(bdd, &heuristic, config, profiler, !no_reorder, peek_set,
                        pause_on_backtrack);
    return engine.search();
  } break;
  }

  assert(false && "Unknown heuristic");
}

std::string nf_name_from_bdd(const std::string &bdd_fname) {
  std::string nf_name = bdd_fname;
  nf_name = nf_name.substr(nf_name.find_last_of("/") + 1);
  nf_name = nf_name.substr(0, nf_name.find_last_of("."));
  return nf_name;
}

int main(int argc, char **argv) {
  CLI::App app{"Synapse"};

  std::filesystem::path input_bdd_file;
  std::filesystem::path config_file;
  std::filesystem::path out_dir;
  HeuristicOption heuristic_opt;
  std::vector<ep_id_t> peek;
  std::filesystem::path bdd_profile;
  u32 seed = 0;
  bool no_reorder = false;
  bool show_prof = false;
  bool show_ep = false;
  bool show_ss = false;
  bool show_bdd = false;
  bool pause_on_backtrack = false;
  bool not_greedy = false;
  bool verbose = false;

  app.add_option("--in", input_bdd_file, "Input file for BDD deserialization.")
      ->required();
  app.add_option("--config", config_file, "Configuration file.")->required();
  app.add_option("--out", out_dir, "Output directory for every generated file.");
  app.add_option("--heuristic", heuristic_opt, "Chosen heuristic.")
      ->transform(CLI::CheckedTransformer(heuristic_opt_converter, CLI::ignore_case))
      ->required();
  app.add_option("--profile", bdd_profile, "BDD profile JSON.");
  app.add_option("--seed", seed, "Random seed.")->default_val(std::random_device()());
  app.add_option("--peek", peek, "Peek execution plans.");
  app.add_flag("--no-reorder", no_reorder, "Deactivate BDD reordering.");
  app.add_flag("--show-prof", show_prof, "Show NF profiling.");
  app.add_flag("--show-ep", show_ep, "Show winner Execution Plan.");
  app.add_flag("--show-ss", show_ss, "Show the entire search space.");
  app.add_flag("--show-bdd", show_bdd, "Show the BDD's solution.");
  app.add_flag("--backtrack", pause_on_backtrack, "Pause on backtrack.");
  app.add_flag("--not-greedy", not_greedy, "Don't stop on first solution.");
  app.add_flag("-v", verbose, "Verbose mode.");

  CLI11_PARSE(app, argc, argv);

  if (verbose) {
    Log::MINIMUM_LOG_LEVEL = Log::Level::DEBUG;
  } else {
    Log::MINIMUM_LOG_LEVEL = Log::Level::LOG;
  }

  RandomEngine::seed(seed);

  BDD *bdd = new BDD(input_bdd_file);

  toml::table config;
  try {
    config = toml::parse_file(config_file.string());
  } catch (const toml::parse_error &err) {
    PANIC("Parsing failed: %s\n", err.what());
  }

  Profiler profiler = bdd_profile.empty() ? Profiler(bdd) : Profiler(bdd, bdd_profile);

  profiler.debug();

  if (show_prof) {
    ProfilerViz::visualize(bdd, profiler, true);
  }

  // std::string nf_name = nf_name_from_bdd(InputBDDFile);
  search_report_t report = search(bdd, config, profiler, heuristic_opt, peek, no_reorder,
                                  pause_on_backtrack, not_greedy);

  if (show_ep) {
    EPViz::visualize(report.solution.ep, false);
  }

  if (show_ss) {
    SSVisualizer::visualize(report.solution.search_space, report.solution.ep, false);
  }

  if (show_bdd) {
    // BDDViz::visualize(report.solution.ep->get_bdd(), false);
    ProfilerViz::visualize(report.solution.ep->get_bdd(),
                           report.solution.ep->get_ctx().get_profiler(), false);
  }

  report.solution.ep->get_ctx().debug();

  Log::log() << "\n";
  Log::log() << "Params:\n";
  Log::log() << "  Heuristic:        " << report.config.heuristic << "\n";
  Log::log() << "  Random seed:      " << seed << "\n";
  Log::log() << "Search:\n";
  Log::log() << "  Search time:      " << report.meta.elapsed_time << " s\n";
  Log::log() << "  SS size:          " << int2hr(report.meta.ss_size) << "\n";
  Log::log() << "  Steps:            " << int2hr(report.meta.steps) << "\n";
  Log::log() << "  Backtracks:       " << int2hr(report.meta.backtracks) << "\n";
  Log::log() << "  Branching factor: " << report.meta.branching_factor << "\n";
  Log::log() << "  Avg BDD size:     " << int2hr(report.meta.avg_bdd_size) << "\n";
  Log::log() << "  Solutions:        " << int2hr(report.meta.solutions) << "\n";
  Log::log() << "Winner EP:\n";
  Log::log() << "  Winner:           " << report.solution.score << "\n";
  Log::log() << "  Throughput:       " << report.solution.tput_estimation << "\n";
  Log::log() << "  Speculation:      " << report.solution.tput_speculation << "\n";
  Log::log() << "\n";

  if (!out_dir.empty()) {
    synthesize(report.solution.ep, out_dir);
  }

  if (report.solution.ep) {
    delete report.solution.ep;
  }

  if (report.solution.search_space) {
    delete report.solution.search_space;
  }

  delete bdd;

  return 0;
}
