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
#include <nlohmann/json.hpp>

std::string nf_name_from_bdd(const std::string &bdd_fname) {
  std::string nf_name = bdd_fname;
  nf_name             = nf_name.substr(nf_name.find_last_of("/") + 1);
  nf_name             = nf_name.substr(0, nf_name.find_last_of("."));
  return nf_name;
}

LibSynapse::targets_config_t parse_targets_config(const std::filesystem::path &targets_config_file) {
  toml::table config;

  try {
    config = toml::parse_file(targets_config_file.string());
  } catch (const toml::parse_error &err) {
    panic("Parsing failed: %s\n", err.what());
  }

  assert(config.contains("switch") && "Switch configuration not found");
  assert(config["switch"].as_table()->contains("arch") && "Arch configuration not found");

  LibSynapse::targets_config_t targets_config;

  targets_config.tofino_config.properties = {
      .total_ports                                = static_cast<int>(config["switch"]["front_panel_ports"].as_array()->size()),
      .total_recirc_ports                         = static_cast<int>(config["switch"]["recirculation_ports"].as_array()->size()),
      .max_packet_bytes_in_condition              = *config["switch"]["arch"]["max_packet_bytes_in_condition"].value<bytes_t>(),
      .pipes                                      = *config["switch"]["arch"]["pipes"].value<int>(),
      .stages                                     = *config["switch"]["arch"]["stages"].value<int>(),
      .sram_per_stage                             = *config["switch"]["arch"]["sram_per_stage"].value<bits_t>(),
      .tcam_per_stage                             = *config["switch"]["arch"]["tcam_per_stage"].value<bits_t>(),
      .map_ram_per_stage                          = *config["switch"]["arch"]["map_ram_per_stage"].value<bits_t>(),
      .max_logical_tcam_tables_per_stage          = *config["switch"]["arch"]["max_logical_tcam_tables_per_stage"].value<int>(),
      .max_logical_sram_and_tcam_tables_per_stage = *config["switch"]["arch"]["max_logical_sram_and_tcam_tables_per_stage"].value<int>(),
      .phv_size                                   = *config["switch"]["arch"]["phv_size"].value<bits_t>(),
      .phv_8bit_containers                        = *config["switch"]["arch"]["phv_8bit_containers"].value<int>(),
      .phv_16bit_containers                       = *config["switch"]["arch"]["phv_16bit_containers"].value<int>(),
      .phv_32bit_containers                       = *config["switch"]["arch"]["phv_32bit_containers"].value<int>(),
      .packet_buffer_size                         = *config["switch"]["arch"]["packet_buffer_size"].value<bits_t>(),
      .exact_match_xbar_per_stage                 = *config["switch"]["arch"]["exact_match_xbar_per_stage"].value<bits_t>(),
      .max_exact_match_keys                       = *config["switch"]["arch"]["max_exact_match_keys"].value<int>(),
      .ternary_match_xbar                         = *config["switch"]["arch"]["ternary_match_xbar"].value<bits_t>(),
      .max_ternary_match_keys                     = *config["switch"]["arch"]["max_ternary_match_keys"].value<int>(),
      .max_salu_size                              = *config["switch"]["arch"]["max_salu_size"].value<bits_t>(),
      .max_digests                                = *config["switch"]["arch"]["max_digests"].value<u8>(),
      .min_expiration_time                        = *config["switch"]["arch"]["min_expiration_time_ms"].value<time_ms_t>(),
  };

  std::unordered_set<u16> nf_devices;
  std::unordered_set<u16> front_panel_ports;

  for (auto &&elem : *config["switch"]["front_panel_ports"].as_array()) {
    int virtual_port  = *elem.at_path("vport").value<int>();
    int physical_port = *elem.at_path("pport").value<int>();
    bps_t capacity    = *elem.at_path("capacity_bps").value<bps_t>();

    if (virtual_port < 0) {
      continue;
    }

    if (nf_devices.find(virtual_port) != nf_devices.end()) {
      panic("Invalid config file: repeated vport %d", virtual_port);
    }

    if (front_panel_ports.find(physical_port) != front_panel_ports.end()) {
      panic("Invalid config file: repeated pport %d", physical_port);
    }

    nf_devices.insert(virtual_port);
    front_panel_ports.insert(physical_port);

    LibSynapse::Tofino::tofino_port_t port{
        .nf_device        = static_cast<u16>(virtual_port),
        .front_panel_port = static_cast<u16>(physical_port),
        .capacity         = capacity,
    };

    targets_config.tofino_config.ports.push_back(port);
  }

  std::unordered_set<u16> dev_ports;

  for (auto &&elem : *config["switch"]["recirculation_ports"].as_array()) {
    int dev_port   = *elem.at_path("dev_port").value<int>();
    bps_t capacity = *elem.at_path("capacity_bps").value<bps_t>();

    if (dev_port < 0) {
      continue;
    }

    if (dev_ports.find(dev_port) != dev_ports.end()) {
      panic("Invalid config file: repeated dev port %d", dev_port);
    }

    dev_ports.insert(dev_port);

    LibSynapse::Tofino::tofino_recirculation_port_t recirc_port{
        .dev_port = static_cast<u16>(dev_port),
        .capacity = capacity,
    };

    targets_config.tofino_config.recirculation_ports.push_back(recirc_port);
  }

  targets_config.controller_capacity = *config["controller"]["capacity_pps"].value<pps_t>();

  if (targets_config.tofino_config.properties.total_recirc_ports != static_cast<int>(targets_config.tofino_config.recirculation_ports.size())) {
    panic("Invalid config file: total_recirc_ports size does not match the number of provided recirculation ports.");
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
  u32 seed;
  bool assume_uniform_forwarding_distribution{false};
  bool show_prof{false};
  bool show_ep{false};
  bool show_ss{false};
  bool show_bdd{false};
  bool skip_synthesis{false};
  bool dry_run{false};

  void print() const {
    const LibSynapse::targets_config_t targets_config = parse_targets_config(targets_config_file);
    const LibSynapse::Targets targets(targets_config);

    std::cout << "====================== Args ======================\n";
    std::cout << "Input BDD file:       " << input_bdd_file.string() << "\n";
    std::cout << "Output directory:     " << out_dir.string() << "\n";
    std::cout << "Name:                 " << name << "\n";
    std::cout << "Targets config:       " << targets_config_file.string() << "\n";
    std::cout << "Heuristic:            " << LibSynapse::heuristic_opt_to_str.at(heuristic_opt) << "\n";
    std::cout << "Profile file:         " << profile_file.string() << "\n";
    std::cout << "Seed:                 " << seed << "\n";
    std::cout << "Targets:              ";
    for (const LibSynapse::TargetView &target : targets.get_view().elements) {
      std::cout << target.type << " (" << target.module_factories.size() << " modules) ";
    }
    std::cout << "\n";
    std::cout << "Profiler:\n";
    std::cout << "  Assume uniform fwd: " << assume_uniform_forwarding_distribution << "\n";
    std::cout << "Search:\n";
    std::cout << "  No reorder:         " << search_config.no_reorder << "\n";
    std::cout << "  Peek:               [";
    for (size_t i = 0; i < search_config.peek.size(); i++) {
      if (i != 0) {
        std::cout << ",";
      }
      std::cout << search_config.peek[i];
    }
    std::cout << "]\n";
    std::cout << "  Pause on BT:        " << search_config.pause_and_show_on_backtrack << "\n";
    std::cout << "  Not greedy:         " << search_config.not_greedy << "\n";
    std::cout << "Debug:\n";
    std::cout << "  Show prof:          " << show_prof << "\n";
    std::cout << "  Show EP:            " << show_ep << "\n";
    std::cout << "  Show SS:            " << show_ss << "\n";
    std::cout << "  Show BDD:           " << show_bdd << "\n";
    std::cout << "Skip synthesis:       " << skip_synthesis << "\n";
    std::cout << "Dry run:              " << dry_run << "\n";
    std::cout << "=================================================\n";
  }
};

void dump_final_report(const args_t &args, const LibSynapse::search_report_t &search_report) {
  const std::filesystem::path out_report_fpath = args.out_dir / (args.name + ".json");

  nlohmann::json report_json;
  report_json["name"]                = args.name;
  report_json["bdd"]                 = args.input_bdd_file.filename().string();
  report_json["targets_config_file"] = args.targets_config_file.filename().string();
  report_json["profile_file"]        = args.profile_file.filename().string();
  report_json["seed"]                = args.seed;
  report_json["heuristic"]           = LibSynapse::heuristic_opt_to_str.at(args.heuristic_opt);

  report_json["heuristic"] = nlohmann::json::object();

  report_json["heuristic"]["score"] = nlohmann::json::array();
  for (const auto &score_value : search_report.score.values) {
    report_json["heuristic"]["score"].push_back(score_value);
  }

  report_json["heuristic"]["meta"] = nlohmann::json::array();
  for (const LibSynapse::heuristic_metadata_t &meta : search_report.heuristic_meta) {
    nlohmann::json meta_json;
    meta_json["name"]        = meta.name;
    meta_json["description"] = meta.description;
    report_json["heuristic"]["meta"].push_back(meta_json);
  }

  report_json["tput_estimation_pps"] = search_report.tput_estimation_pps;
  report_json["tput_estimation_bps"] = search_report.tput_estimation_bps;

  report_json["search_meta"] = {
      {"elapsed_time_seconds", search_report.meta.elapsed_time},
      {"steps", search_report.meta.steps},
      {"backtracks", search_report.meta.backtracks},
      {"ss_size", search_report.meta.ss_size},
      {"unfinished_eps", search_report.meta.unfinished_eps},
      {"finished_eps", search_report.meta.finished_eps},
      {"avg_bdd_size", search_report.meta.avg_bdd_size},
      {"branching_factor", search_report.meta.branching_factor},
      {"total_ss_size_estimation", search_report.meta.total_ss_size_estimation},
      {"avg_children_per_node", nlohmann::json::array()},
  };

  for (const auto &[node_id, avg_children] : search_report.meta.avg_children_per_node) {
    nlohmann::json avg_children_json;
    avg_children_json["node_id"] = node_id;
    avg_children_json["avg"]     = avg_children;
    report_json["search_meta"]["avg_children_per_node"].push_back(avg_children_json);
  }

  report_json["implementations"] = nlohmann::json::array();
  for (const auto &[ds_addr, ds_impl] : search_report.ep->get_ctx().get_ds_impls()) {
    nlohmann::json ds_impl_json;
    ds_impl_json["addr"]           = ds_addr;
    ds_impl_json["implementation"] = LibSynapse::ds_impl_to_string(ds_impl);
    report_json["implementations"].push_back(ds_impl_json);
  }

  std::ofstream out_report(out_report_fpath);
  if (!out_report.is_open()) {
    panic("Failed to open output report file: %s", out_report_fpath.string().c_str());
  }

  out_report << std::setw(2) << report_json << "\n";
  out_report.close();
}

void dump_final_hr_report(const args_t &args, const LibSynapse::search_report_t &search_report) {
  const std::filesystem::path out_hr_report_fpath = args.out_dir / (args.name + ".txt");

  std::ofstream out_hr_report(out_hr_report_fpath);
  if (!out_hr_report.is_open()) {
    panic("Failed to open output report file: %s", out_hr_report_fpath.string().c_str());
  }

  out_hr_report << "===================== Synapse Report ====================\n";
  out_hr_report << "Args:\n";
  out_hr_report << "  Input BDD file:     " << args.input_bdd_file.filename().string() << "\n";
  out_hr_report << "  Name:               " << args.name << "\n";
  out_hr_report << "  Targets config:     " << args.targets_config_file.filename().string() << "\n";
  out_hr_report << "  Heuristic:          " << LibSynapse::heuristic_opt_to_str.at(args.heuristic_opt) << "\n";
  out_hr_report << "  Profile file:       " << args.profile_file.filename().string() << "\n";
  out_hr_report << "  Seed:               " << args.seed << "\n";
  out_hr_report << "  Targets:\n";
  const LibSynapse::targets_config_t targets_config = parse_targets_config(args.targets_config_file);
  const LibSynapse::Targets targets(targets_config);
  for (const LibSynapse::TargetView &target : targets.get_view().elements) {
    out_hr_report << "    " << target.type << " (" << target.module_factories.size() << " modules)\n";
  }
  out_hr_report << "  No reorder:         " << args.search_config.no_reorder << "\n";
  out_hr_report << "  Not greedy:         " << args.search_config.not_greedy << "\n";
  out_hr_report << "  Assume uniform fwd: " << args.assume_uniform_forwarding_distribution << "\n";
  out_hr_report << "\n";

  out_hr_report << "Winner:\n";
  out_hr_report << "  Score: " << search_report.score << "\n";
  for (const LibSynapse::heuristic_metadata_t &meta : search_report.heuristic_meta) {
    out_hr_report << "  " << meta.name << ": " << meta.description << "\n";
  }
  out_hr_report << "\n";

  out_hr_report << "Stateful Implementations:\n";
  for (const auto &[ds_addr, ds_impl] : search_report.ep->get_ctx().get_ds_impls()) {
    out_hr_report << "  " << ds_addr << ": " << ds_impl << "\n";
  }
  out_hr_report << "\n";

  out_hr_report << "Search Metadata:\n";
  out_hr_report << "  Elapsed time:       " << search_report.meta.elapsed_time << " seconds\n";
  out_hr_report << "  Steps:              " << LibCore::int2hr(search_report.meta.steps) << "\n";
  out_hr_report << "  Backtracks:         " << LibCore::int2hr(search_report.meta.backtracks) << "\n";
  out_hr_report << "  Search space size:  " << LibCore::int2hr(search_report.meta.ss_size) << "\n";
  out_hr_report << "  Unfinished EPs:     " << LibCore::int2hr(search_report.meta.unfinished_eps) << "\n";
  out_hr_report << "  Finished EPs:       " << LibCore::int2hr(search_report.meta.finished_eps) << "\n";
  out_hr_report << "  Avg BDD size:       " << LibCore::int2hr(search_report.meta.avg_bdd_size) << "\n";
  out_hr_report << "  Branching factor:   " << LibCore::int2hr(search_report.meta.branching_factor) << "\n";
  out_hr_report << "  Total SS size est.: " << LibCore::int2hr(search_report.meta.total_ss_size_estimation) << "\n";
  out_hr_report << "  Avg children per node:\n";
  for (const auto &[node_id, avg_children] : search_report.meta.avg_children_per_node) {
    out_hr_report << "    Node " << node_id << ": " << avg_children << "\n";
  }
  out_hr_report << "\n";

  out_hr_report << "========================================================\n";
  out_hr_report.close();
}

int main(int argc, char **argv) {
  CLI::App app{"Synapse"};

  args_t args;

  app.add_option("--in", args.input_bdd_file, "Input file for BDD deserialization.")->required();
  app.add_option("--out", args.out_dir, "Output directory for every generated file.");
  app.add_option("--name", args.name, "Synthesized filenames (without extensions) (defaults to \"synapse-{bdd filename}\").");
  app.add_option("--config", args.targets_config_file, "Configuration file.")->required();
  app.add_option("--heuristic", args.heuristic_opt, "Chosen heuristic.")
      ->transform(CLI::CheckedTransformer(LibSynapse::str_to_heuristic_opt, CLI::ignore_case))
      ->required();
  app.add_option("--profile", args.profile_file, "BDD profile file JSON.");
  app.add_option("--seed", args.seed, "Random seed.")->default_val(std::random_device()());
  app.add_option("--peek", args.search_config.peek, "Peek execution plans.");
  app.add_flag("--no-reorder", args.search_config.no_reorder, "Deactivate BDD reordering.");
  app.add_flag("--show-prof", args.show_prof, "Show NF profiling.");
  app.add_flag("--show-ep", args.show_ep, "Show winner Execution Plan.");
  app.add_flag("--show-ss", args.show_ss, "Show the entire search space.");
  app.add_flag("--show-bdd", args.show_bdd, "Show the BDD's solution.");
  app.add_flag("--backtrack", args.search_config.pause_and_show_on_backtrack, "Pause on backtrack.");
  app.add_flag("--not-greedy", args.search_config.not_greedy, "Don't stop on first solution.");
  app.add_flag("--assume-uniform-fwd", args.assume_uniform_forwarding_distribution,
               "Assume uniform distribution of forwarding decisions across all active ports.");
  app.add_flag("--skip-synthesis", args.skip_synthesis, "Skip synthesis step (only search).");
  app.add_flag("--dry-run", args.dry_run, "Don't run search.");

  CLI11_PARSE(app, argc, argv);

  if (args.name.empty()) {
    args.name = "synapse";
    args.name += "-" + nf_name_from_bdd(args.input_bdd_file);
    args.name += "-" + LibSynapse::heuristic_opt_to_str.at(args.heuristic_opt);
    args.name += "-" + std::to_string(args.seed);
    if (!args.profile_file.empty()) {
      args.name += "-" + args.profile_file.stem().string();
    }
  }

  if (args.dry_run) {
    args.print();
    return 0;
  }

  LibCore::SingletonRandomEngine::seed(args.seed);
  LibCore::SymbolManager symbol_manager;
  const LibBDD::BDD bdd(args.input_bdd_file, &symbol_manager);
  const LibSynapse::targets_config_t targets_config = parse_targets_config(args.targets_config_file);
  const LibBDD::bdd_profile_t bdd_profile =
      args.profile_file.empty() ? LibBDD::build_random_bdd_profile(&bdd) : LibBDD::parse_bdd_profile(args.profile_file);
  const LibSynapse::Profiler profiler = LibSynapse::Profiler(&bdd, bdd_profile, args.assume_uniform_forwarding_distribution);

  if (args.show_prof) {
    profiler.debug();
    LibSynapse::ProfilerViz::visualize(&bdd, profiler, true);
  }

  LibSynapse::SearchEngine engine(bdd, args.heuristic_opt, profiler, targets_config, args.search_config);
  const LibSynapse::search_report_t report = engine.search();

  if (args.show_ep) {
    LibSynapse::EPViz::visualize(report.ep.get(), false);
  }

  if (args.show_ss) {
    LibSynapse::SSViz::visualize(report.search_space.get(), report.ep.get(), false);
  }

  if (args.show_bdd) {
    LibSynapse::ProfilerViz::visualize(report.ep->get_bdd(), report.ep->get_ctx().get_profiler(), false);
  }

  if (!args.out_dir.empty()) {
    const std::filesystem::path bdd_fpath = args.out_dir / (args.name + "-bdd.dot");
    const std::filesystem::path ep_fpath  = args.out_dir / (args.name + "-ep.dot");
    const std::filesystem::path ss_fpath  = args.out_dir / (args.name + "-ss.dot");

    LibSynapse::ProfilerViz::dump_to_file(report.ep->get_bdd(), report.ep->get_ctx().get_profiler(), bdd_fpath);
    LibSynapse::EPViz::dump_to_file(report.ep.get(), ep_fpath);
    LibSynapse::SSViz::dump_to_file(report.search_space.get(), report.ep.get(), ss_fpath);

    if (!args.skip_synthesis) {
      LibSynapse::synthesize(report.ep.get(), args.name, args.out_dir);
    }
  }

  if (!args.out_dir.empty()) {
    dump_final_hr_report(args, report);
    dump_final_report(args, report);
  }

  report.ep->get_ctx().debug();

  std::cout << "Params:\n";
  std::cout << "  BDD file:         " << args.input_bdd_file.filename().string() << "\n";
  std::cout << "  Profile file:     " << args.profile_file.filename().string() << "\n";
  std::cout << "  Config file:      " << args.targets_config_file.filename().string() << "\n";
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
  std::cout << "  Score: " << report.score << "\n";
  for (const LibSynapse::heuristic_metadata_t &meta : report.heuristic_meta) {
    std::cout << "  " << meta.name << ": " << meta.description << "\n";
  }
  std::cout << "\n";

  return 0;
}
