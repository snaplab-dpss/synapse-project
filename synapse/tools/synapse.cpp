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

struct args_t {
  std::filesystem::path input_bdd_file;
  std::filesystem::path out_dir;
  std::string name;
  std::filesystem::path targets_config_file;
  LibSynapse::HeuristicOption heuristic_opt;
  std::filesystem::path profile_file;
  LibSynapse::search_config_t search_config;
  u32 seed{0};
  bool show_prof{false};
  bool show_ep{false};
  bool show_ss{false};
  bool show_bdd{false};
  bool dry_run{false};

  void print() const {
    auto heuristic_to_str = [](LibSynapse::HeuristicOption h) -> std::string {
      for (const auto &[str, opt] : heuristic_opt_converter) {
        if (opt == h) {
          return str;
        }
      }
      return "Unknown";
    };

    LibSynapse::Targets targets(parse_targets_config(targets_config_file));

    std::cout << "====================== Args ======================\n";
    std::cout << "Input BDD file:   " << input_bdd_file << "\n";
    std::cout << "Output directory: " << out_dir << "\n";
    std::cout << "Name:             " << name << "\n";
    std::cout << "Targets config:   " << targets_config_file << "\n";
    std::cout << "Heuristic:        " << heuristic_to_str(heuristic_opt) << "\n";
    std::cout << "Profile file:     " << profile_file << "\n";
    std::cout << "Seed:             " << seed << "\n";
    std::cout << "Targets:          ";
    for (const LibSynapse::TargetView &target : targets.get_view().elements) {
      std::cout << target.type << " (" << target.module_factories.size() << " modules) ";
    }
    std::cout << "\n";
    std::cout << "Search:\n";
    std::cout << "  No reorder:     " << search_config.no_reorder << "\n";
    std::cout << "  Peek:           [";
    for (size_t i = 0; i < search_config.peek.size(); i++) {
      if (i != 0) {
        std::cout << ",";
      }
      std::cout << search_config.peek[i];
    }
    std::cout << "]\n";
    std::cout << "  Pause on BT:    " << search_config.pause_and_show_on_backtrack << "\n";
    std::cout << "  Not greedy:     " << search_config.not_greedy << "\n";
    std::cout << "Debug:\n";
    std::cout << "  Show prof:      " << show_prof << "\n";
    std::cout << "  Show EP:        " << show_ep << "\n";
    std::cout << "  Show SS:        " << show_ss << "\n";
    std::cout << "  Show BDD:       " << show_bdd << "\n";
    std::cout << "Dry run:          " << dry_run << "\n";
    std::cout << "=================================================\n";
  }
};

int main(int argc, char **argv) {
  CLI::App app{"Synapse"};

  args_t args;

  app.add_option("--in", args.input_bdd_file, "Input file for BDD deserialization.")->required();
  app.add_option("--out", args.out_dir, "Output directory for every generated file.");
  app.add_option("--name", args.name, "Synthesized filenames (without extensions) (defaults to \"synapse-{bdd filename}\").");
  app.add_option("--config", args.targets_config_file, "Configuration file.")->required();
  app.add_option("--heuristic", args.heuristic_opt, "Chosen heuristic.")
      ->transform(CLI::CheckedTransformer(heuristic_opt_converter, CLI::ignore_case))
      ->required();
  app.add_option("--profile", args.profile_file, "BDD profile_file JSON.");
  app.add_option("--seed", args.seed, "Random seed.")->default_val(std::random_device()());
  app.add_option("--peek", args.search_config.peek, "Peek execution plans.");
  app.add_flag("--no-reorder", args.search_config.no_reorder, "Deactivate BDD reordering.");
  app.add_flag("--show-prof", args.show_prof, "Show NF profiling.");
  app.add_flag("--show-ep", args.show_ep, "Show winner Execution Plan.");
  app.add_flag("--show-ss", args.show_ss, "Show the entire search space.");
  app.add_flag("--show-bdd", args.show_bdd, "Show the BDD's solution.");
  app.add_flag("--backtrack", args.search_config.pause_and_show_on_backtrack, "Pause on backtrack.");
  app.add_flag("--not-greedy", args.search_config.not_greedy, "Don't stop on first solution.");
  app.add_flag("--dry-run", args.dry_run, "Don't run search.");

  CLI11_PARSE(app, argc, argv);

  if (args.name.empty()) {
    args.name = "synapse-" + nf_name_from_bdd(args.input_bdd_file);
  }

  args.print();

  if (args.dry_run) {
    return 0;
  }

  LibCore::SingletonRandomEngine::seed(args.seed);
  LibCore::SymbolManager symbol_manager;
  LibBDD::BDD bdd               = LibBDD::BDD(args.input_bdd_file, &symbol_manager);
  toml::table targets_config    = parse_targets_config(args.targets_config_file);
  LibSynapse::Profiler profiler = args.profile_file.empty() ? LibSynapse::Profiler(&bdd) : LibSynapse::Profiler(&bdd, args.profile_file);

  if (args.show_prof) {
    profiler.debug();
    LibSynapse::ProfilerViz::visualize(&bdd, profiler, true);
  }

  LibSynapse::SearchEngine engine(bdd, args.heuristic_opt, profiler, targets_config, args.search_config);
  LibSynapse::search_report_t report = engine.search();

  report.ep->get_ctx().debug();

  std::cout << "Params:\n";
  std::cout << "  Heuristic:        " << report.heuristic << "\n";
  std::cout << "  Random seed:      " << args.seed << "\n";
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
  std::cout << "\n";

  if (args.show_ep) {
    LibSynapse::EPViz::visualize(report.ep.get(), false);
  }

  if (args.show_ss) {
    LibSynapse::SSVisualizer::visualize(report.search_space.get(), report.ep.get(), false);
  }

  if (args.show_bdd) {
    LibSynapse::ProfilerViz::visualize(report.ep->get_bdd(), report.ep->get_ctx().get_profiler(), false);
  }

  if (!args.out_dir.empty()) {
    LibSynapse::synthesize(report.ep.get(), args.name, args.out_dir);
    LibSynapse::EPViz::dump_to_file(report.ep.get(), args.out_dir / (args.name + "-ep.dot"));
    LibSynapse::SSVisualizer::dump_to_file(report.search_space.get(), args.out_dir / (args.name + "-ss.dot"));
  }

  return 0;
}
