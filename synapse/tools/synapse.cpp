#include <filesystem>
#include <fstream>
#include <CLI/CLI.hpp>
#include <toml++/toml.hpp>
#include <variant>

#include "../src/bdd/bdd.h"
#include "../src/log.h"
#include "../src/visualizers/ep_visualizer.h"
#include "../src/visualizers/ss_visualizer.h"
#include "../src/visualizers/profiler_visualizer.h"
#include "../src/heuristics/heuristics.h"
#include "../src/search.h"
#include "../src/synthesizers/ep/synthesizers.h"
#include "../src/targets/targets.h"

const std::unordered_map<std::string, HeuristicOption> heuristic_opt_converter{
    {"bfs", HeuristicOption::BFS},         {"dfs", HeuristicOption::DFS},
    {"random", HeuristicOption::RANDOM},   {"gallium", HeuristicOption::GALLIUM},
    {"greedy", HeuristicOption::GREEDY},   {"max-tput", HeuristicOption::MAX_TPUT},
    {"ds-pref", HeuristicOption::DS_PREF},
};

std::string nf_name_from_bdd(const std::string &bdd_fname) {
  std::string nf_name = bdd_fname;
  nf_name = nf_name.substr(nf_name.find_last_of("/") + 1);
  nf_name = nf_name.substr(0, nf_name.find_last_of("."));
  return nf_name;
}

toml::table parse_targets_config(const std::filesystem::path &targets_config_file) {
  toml::table targets_config;
  try {
    targets_config = toml::parse_file(targets_config_file.string());
  } catch (const toml::parse_error &err) {
    PANIC("Parsing failed: %s\n", err.what());
  }
  return targets_config;
}

int main(int argc, char **argv) {
  CLI::App app{"Synapse"};

  std::filesystem::path input_bdd_file;
  std::filesystem::path targets_config_file;
  std::filesystem::path out_dir;
  HeuristicOption heuristic_opt;
  std::filesystem::path bdd_profile;
  search_config_t search_config;
  u32 seed = 0;
  bool show_prof = false;
  bool show_ep = false;
  bool show_ss = false;
  bool show_bdd = false;
  bool verbose = false;

  app.add_option("--in", input_bdd_file, "Input file for BDD deserialization.")
      ->required();
  app.add_option("--config", targets_config_file, "Configuration file.")->required();
  app.add_option("--out", out_dir, "Output directory for every generated file.");
  app.add_option("--heuristic", heuristic_opt, "Chosen heuristic.")
      ->transform(CLI::CheckedTransformer(heuristic_opt_converter, CLI::ignore_case))
      ->required();
  app.add_option("--profile", bdd_profile, "BDD profile JSON.");
  app.add_option("--seed", seed, "Random seed.")->default_val(std::random_device()());
  app.add_option("--peek", search_config.peek, "Peek execution plans.");
  app.add_flag("--no-reorder", search_config.no_reorder, "Deactivate BDD reordering.");
  app.add_flag("--show-prof", show_prof, "Show NF profiling.");
  app.add_flag("--show-ep", show_ep, "Show winner Execution Plan.");
  app.add_flag("--show-ss", show_ss, "Show the entire search space.");
  app.add_flag("--show-bdd", show_bdd, "Show the BDD's solution.");
  app.add_flag("--backtrack", search_config.pause_and_show_on_backtrack,
               "Pause on backtrack.");
  app.add_flag("--not-greedy", search_config.not_greedy, "Don't stop on first solution.");
  app.add_flag("-v", verbose, "Verbose mode.");

  CLI11_PARSE(app, argc, argv);

  if (verbose) {
    Log::MINIMUM_LOG_LEVEL = Log::Level::DEBUG;
  } else {
    Log::MINIMUM_LOG_LEVEL = Log::Level::LOG;
  }

  RandomEngine::seed(seed);
  std::unique_ptr<BDD> bdd = std::make_unique<BDD>(input_bdd_file);
  toml::table targets_config = parse_targets_config(targets_config_file);
  Profiler profiler =
      bdd_profile.empty() ? Profiler(bdd.get()) : Profiler(bdd.get(), bdd_profile);

  if (show_prof) {
    profiler.debug();
    ProfilerViz::visualize(bdd.get(), profiler, true);
  }

  // std::string nf_name = nf_name_from_bdd(InputBDDFile);
  SearchEngine engine(bdd.get(), heuristic_opt, profiler, targets_config, search_config);
  search_report_t report = engine.search();

  if (show_ep) {
    EPViz::visualize(report.ep.get(), false);
  }

  if (show_ss) {
    SSVisualizer::visualize(report.search_space.get(), report.ep.get(), false);
  }

  if (show_bdd) {
    // BDDViz::visualize(report.solution.ep->get_bdd(), false);
    ProfilerViz::visualize(report.ep->get_bdd(), report.ep->get_ctx().get_profiler(),
                           false);
  }

  report.ep->get_ctx().debug();

  Log::log() << "\n";
  Log::log() << "Params:\n";
  Log::log() << "  Heuristic:        " << report.heuristic << "\n";
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
  Log::log() << "  Winner:           " << report.score << "\n";
  Log::log() << "  Throughput:       " << report.tput_estimation << "\n";
  Log::log() << "  Speculation:      " << report.tput_speculation << "\n";
  Log::log() << "\n";

  if (!out_dir.empty()) {
    synthesize(report.ep.get(), out_dir);
  }

  return 0;
}
