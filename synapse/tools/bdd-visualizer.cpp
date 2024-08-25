#include <filesystem>
#include <fstream>
#include <CLI/CLI.hpp>

#include "../src/bdd/bdd.h"

int main(int argc, char **argv) {
  CLI::App app{"Visualize BDD"};

  std::filesystem::path input_bdd_file;
  std::filesystem::path output_dot_file;
  std::filesystem::path bdd_profile_file;
  bool show;

  app.add_option("--in", input_bdd_file, "Input file for BDD deserialization.");
  app.add_option("--out", output_dot_file, "Output dot file.");
  app.add_option("--report", bdd_profile_file, "BDD profile file.");
  app.add_flag("--show", show, "Render dot file.")->default_val(false);

  CLI11_PARSE(app, argc, argv);

  BDD *bdd = new BDD(input_bdd_file);

  if (!bdd_profile_file.empty()) {
    bdd_profile_t profile = parse_bdd_profile(bdd_profile_file);

    if (!output_dot_file.empty()) {
      BDDProfileVisualizer generator(output_dot_file, profile.counters);
      generator.visit(bdd);
      generator.write();
    }

    if (show) {
      BDDProfileVisualizer::visualize(bdd, profile.counters, false);
    }
  } else {
    if (!output_dot_file.empty()) {
      bdd_visualizer_opts_t opts;
      opts.fname = output_dot_file.string();

      BDDVisualizer generator(opts);
      generator.visit(bdd);
      generator.write();
    }

    if (show) {
      BDDVisualizer::visualize(bdd, false);
    }
  }

  delete bdd;

  return 0;
}
