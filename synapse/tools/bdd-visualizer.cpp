#include <filesystem>
#include <fstream>
#include <CLI/CLI.hpp>

#include "../src/bdd/bdd.hpp"

using namespace synapse;

int main(int argc, char **argv) {
  CLI::App app{"Visualize BDD"};

  std::filesystem::path input_bdd_file;
  std::filesystem::path output_dot_file;
  std::filesystem::path bdd_profile_file;
  bool show{false};

  app.add_option("--in", input_bdd_file, "Input file for BDD deserialization.")->required();
  app.add_option("--out", output_dot_file, "Output dot file.");
  app.add_option("--profile", bdd_profile_file, "BDD profile file.");
  app.add_flag("--show", show, "Render dot file.");

  CLI11_PARSE(app, argc, argv);

  SymbolManager manager;
  std::unique_ptr<BDD> bdd = std::make_unique<BDD>(input_bdd_file, &manager);

  if (!bdd_profile_file.empty()) {
    bdd_profile_t profile = parse_bdd_profile(bdd_profile_file);

    for (const auto &map : profile.stats_per_map) {
      std::cerr << "Map " << map.first << std::endl;
      for (size_t k = 10; k <= 1000000; k *= 10) {
        std::cerr << "Top-k=" << k << " churn=" << profile.churn_top_k_flows(map.first, k) << " fpm"
                  << " hr=" << profile.churn_hit_rate_top_k_flows(map.first, k) << "\n";
      }
    }

    if (!output_dot_file.empty()) {
      BDDProfileVisualizer generator(output_dot_file, profile);
      generator.visit(bdd.get());
      generator.write();
    }

    if (show) {
      BDDProfileVisualizer::visualize(bdd.get(), profile, false);
    }
  } else {
    if (!output_dot_file.empty()) {
      bdd_visualizer_opts_t opts;
      opts.fname = output_dot_file.string();

      BDDViz generator(opts);
      generator.visit(bdd.get());
      generator.write();
    }

    if (show) {
      BDDViz::visualize(bdd.get(), false);
    }
  }

  return 0;
}
