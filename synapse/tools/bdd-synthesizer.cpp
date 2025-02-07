#include <LibBDD/BDD.h>
#include <LibBDD/Visitors/BDDVisualizer.h>
#include <LibBDD/Visitors/BDDProfileVisualizer.h>
#include <LibBDD/Visitors/BDDSynthesizer.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <CLI/CLI.hpp>

const std::unordered_map<std::string, LibBDD::BDDSynthesizerTarget> target_converter{
    {"nf", LibBDD::BDDSynthesizerTarget::NF},
    {"profiler", LibBDD::BDDSynthesizerTarget::PROFILER},
};

int main(int argc, char **argv) {
  CLI::App app{"BDD synthesizer."};

  std::filesystem::path input_bdd_file;
  std::filesystem::path output_file;
  LibBDD::BDDSynthesizerTarget target;

  app.add_option("--in", input_bdd_file, "Input file for BDD deserialization.")->required();
  app.add_option("--out", output_file,
                 "Output C++ file of the syntethized code. If omited, code "
                 "will be dumped to stdout.");
  app.add_option("--target", target, "Chosen target.")
      ->default_val(LibBDD::BDDSynthesizerTarget::NF)
      ->transform(CLI::CheckedTransformer(target_converter, CLI::ignore_case));

  CLI11_PARSE(app, argc, argv);

  LibCore::SymbolManager symbol_manager;
  LibBDD::BDD bdd(input_bdd_file, &symbol_manager);

  if (output_file.empty()) {
    std::ostream os(std::cout.rdbuf());
    LibBDD::BDDSynthesizer synthesizer(target, os);
    synthesizer.synthesize(&bdd);
  } else {
    assert(!std::filesystem::is_directory(output_file));
    if (output_file.has_parent_path()) {
      std::filesystem::create_directories(output_file.parent_path());
    }
    std::ofstream os(output_file);
    LibBDD::BDDSynthesizer synthesizer(target, os);
    synthesizer.synthesize(&bdd);
    std::cerr << "Output written to " << output_file << ".\n";
  }

  return 0;
}
