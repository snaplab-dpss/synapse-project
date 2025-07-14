#include <LibBDD/BDD.h>
#include <LibBDD/Visitors/BDDVisualizer.h>
#include <LibBDD/Visitors/BDDProfileVisualizer.h>
#include <LibBDD/Visitors/BDDSynthesizer.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <CLI/CLI.hpp>

using namespace LibCore;
using namespace LibBDD;

const std::unordered_map<std::string, BDDSynthesizerTarget> target_converter{
    {"nf", BDDSynthesizerTarget::NF},
    {"profiler", BDDSynthesizerTarget::Profiler},
};

int main(int argc, char **argv) {
  CLI::App app{"BDD synthesizer."};

  std::filesystem::path input_bdd_file;
  std::filesystem::path output_file;
  BDDSynthesizerTarget target;

  app.add_option("--in", input_bdd_file, "Input file for BDD deserialization.")->required();
  app.add_option("--out", output_file, "Output C++ file of the syntethized code.")->required();
  app.add_option("--target", target, "Chosen target.")
      ->default_val(BDDSynthesizerTarget::NF)
      ->transform(CLI::CheckedTransformer(target_converter, CLI::ignore_case));

  CLI11_PARSE(app, argc, argv);

  SymbolManager symbol_manager;
  BDD bdd(input_bdd_file, &symbol_manager);

  assert(!std::filesystem::is_directory(output_file));
  if (output_file.has_parent_path()) {
    std::filesystem::create_directories(output_file.parent_path());
  }
  BDDSynthesizer synthesizer(&bdd, target, output_file);
  synthesizer.synthesize();
  std::cerr << "Output written to " << output_file << ".\n";

  return 0;
}
