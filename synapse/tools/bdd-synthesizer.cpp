#include <filesystem>
#include <fstream>
#include <iostream>
#include <CLI/CLI.hpp>

#include "../src/bdd/bdd.h"
#include "../src/synthesizers/bdd/synthesizer.h"

const std::unordered_map<std::string, BDDSynthesizerTarget> target_converter{
    {"nf", BDDSynthesizerTarget::NF},
    {"profiler", BDDSynthesizerTarget::PROFILER},
};

int main(int argc, char **argv) {
  CLI::App app{"BDD synthesizer."};

  std::filesystem::path input_bdd_file;
  std::filesystem::path output_file;
  BDDSynthesizerTarget target;

  app.add_option("--in", input_bdd_file, "Input file for BDD deserialization.")
      ->required();
  app.add_option("--out", output_file,
                 "Output C++ file of the syntethized code. If omited, code "
                 "will be dumped to stdout.");
  app.add_option("--target", target, "Chosen target.")
      ->default_val(BDDSynthesizerTarget::NF)
      ->transform(CLI::CheckedTransformer(target_converter, CLI::ignore_case));

  CLI11_PARSE(app, argc, argv);

  BDD *bdd = new BDD(input_bdd_file);

  if (output_file.empty()) {
    std::ostream os(std::cout.rdbuf());
    BDDSynthesizer synthesizer(target, os);
    synthesizer.visit(bdd);
  } else {
    std::ofstream os(output_file);
    BDDSynthesizer synthesizer(target, os);
    synthesizer.visit(bdd);
    std::cerr << "Output written to " << output_file << ".\n";
  }

  return 0;
}
