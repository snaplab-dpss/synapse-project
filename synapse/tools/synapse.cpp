#include <LibSynapse/Search.h>
#include <LibSynapse/Synthesizers/Synthesizers.h>
#include <LibSynapse/Visualizers/EPVisualizer.h>
#include <LibSynapse/Visualizers/SSVisualizer.h>
#include <LibSynapse/Visualizers/ProfilerVisualizer.h>
#include <LibCore/Debug.h>

#include <filesystem>
#include <fstream>
#include <CLI/CLI.hpp>
#include <toml++/toml.hpp>

const std::unordered_map<std::string, LibSynapse::HeuristicOption> heuristic_opt_converter{
    {"bfs", LibSynapse::HeuristicOption::BFS},         {"dfs", LibSynapse::HeuristicOption::DFS},
    {"random", LibSynapse::HeuristicOption::RANDOM},   {"gallium", LibSynapse::HeuristicOption::GALLIUM},
    {"greedy", LibSynapse::HeuristicOption::GREEDY},   {"max-tput", LibSynapse::HeuristicOption::MAX_TPUT},
    {"ds-pref", LibSynapse::HeuristicOption::DS_PREF},
};

std::string nf_name_from_bdd(const std::string &bdd_fname) {
  std::string nf_name = bdd_fname;
  nf_name             = nf_name.substr(nf_name.find_last_of("/") + 1);
  nf_name             = nf_name.substr(0, nf_name.find_last_of("."));
  return nf_name;
}

toml::table parse_targets_config(const std::filesystem::path &targets_config_file) {
  toml::table targets_config;
  try {
    targets_config = toml::parse_file(targets_config_file.string());
  } catch (const toml::parse_error &err) {
    panic("Parsing failed: %s\n", err.what());
  }
  return targets_config;
}

int main(int argc, char **argv) {
  CLI::App app{"Synapse"};

  std::filesystem::path input_bdd_file;
  std::filesystem::path targets_config_file;
  std::filesystem::path out_dir;
  LibSynapse::HeuristicOption heuristic_opt;
  std::filesystem::path profile_file;
  LibSynapse::search_config_t search_config;
  u32 seed       = 0;
  bool show_prof = false;
  bool show_ep   = false;
  bool show_ss   = false;
  bool show_bdd  = false;

  app.add_option("--in", input_bdd_file, "Input file for BDD deserialization.")->required();
  app.add_option("--config", targets_config_file, "Configuration file.")->required();
  app.add_option("--out", out_dir, "Output directory for every generated file.");
  app.add_option("--heuristic", heuristic_opt, "Chosen heuristic.")
      ->transform(CLI::CheckedTransformer(heuristic_opt_converter, CLI::ignore_case))
      ->required();
  app.add_option("--profile", profile_file, "BDD profile_file JSON.");
  app.add_option("--seed", seed, "Random seed.")->default_val(std::random_device()());
  app.add_option("--peek", search_config.peek, "Peek execution plans.");
  app.add_flag("--no-reorder", search_config.no_reorder, "Deactivate BDD reordering.");
  app.add_flag("--show-prof", show_prof, "Show NF profiling.");
  app.add_flag("--show-ep", show_ep, "Show winner Execution Plan.");
  app.add_flag("--show-ss", show_ss, "Show the entire search space.");
  app.add_flag("--show-bdd", show_bdd, "Show the BDD's solution.");
  app.add_flag("--backtrack", search_config.pause_and_show_on_backtrack, "Pause on backtrack.");
  app.add_flag("--not-greedy", search_config.not_greedy, "Don't stop on first solution.");

  CLI11_PARSE(app, argc, argv);

  LibCore::SingletonRandomEngine::seed(seed);
  LibCore::SymbolManager symbol_manager;
  LibBDD::BDD bdd               = LibBDD::BDD(input_bdd_file, &symbol_manager);
  toml::table targets_config    = parse_targets_config(targets_config_file);
  LibSynapse::Profiler profiler = profile_file.empty() ? LibSynapse::Profiler(&bdd) : LibSynapse::Profiler(&bdd, profile_file);

  if (show_prof) {
    profiler.debug();
    LibSynapse::ProfilerViz::visualize(&bdd, profiler, true);
  }

  // std::string nf_name = nf_name_from_bdd(InputBDDFile);
  LibSynapse::SearchEngine engine(bdd, heuristic_opt, profiler, targets_config, search_config);
  LibSynapse::search_report_t report = engine.search();

  report.ep->get_ctx().debug();

  std::cout << "Params:\n";
  std::cout << "  Heuristic:        " << report.heuristic << "\n";
  std::cout << "  Random seed:      " << seed << "\n";
  std::cout << "Search:\n";
  std::cout << "  Search time:      " << report.meta.elapsed_time << " s\n";
  std::cout << "  SS size:          " << LibCore::int2hr(report.meta.ss_size) << "\n";
  std::cout << "  Steps:            " << LibCore::int2hr(report.meta.steps) << "\n";
  std::cout << "  Backtracks:       " << LibCore::int2hr(report.meta.backtracks) << "\n";
  std::cout << "  Branching factor: " << report.meta.branching_factor << "\n";
  std::cout << "  Avg BDD size:     " << LibCore::int2hr(report.meta.avg_bdd_size) << "\n";
  std::cout << "  Unfinished EPs:   " << LibCore::int2hr(report.meta.unfinished_eps) << "\n";
  std::cout << "  Solutions:        " << LibCore::int2hr(report.meta.finished_eps) << "\n";
  std::cout << "Winner EP:\n";
  std::cout << "  Winner:           " << report.score << "\n";
  std::cout << "  Throughput:       " << report.tput_estimation << "\n";
  std::cout << "  Speculation:      " << report.tput_speculation << "\n";
  std::cout << "\n";

  if (show_ep) {
    LibSynapse::EPViz::visualize(report.ep.get(), false);
  }

  if (show_ss) {
    LibSynapse::SSVisualizer::visualize(report.search_space.get(), report.ep.get(), false);
  }

  if (show_bdd) {
    // BDDViz::visualize(report.solution.ep->get_bdd(), false);
    LibSynapse::ProfilerViz::visualize(report.ep->get_bdd(), report.ep->get_ctx().get_profiler(), false);
  }

  if (!out_dir.empty()) {
    LibSynapse::synthesize(report.ep.get(), out_dir);
  }

  return 0;
}
