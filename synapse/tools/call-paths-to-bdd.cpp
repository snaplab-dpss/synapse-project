#include <LibBDD/BDD.h>
#include <LibBDD/Visitors/PrinterDebug.h>

#include <fstream>
#include <filesystem>
#include <CLI/CLI.hpp>

int main(int argc, char **argv) {
  CLI::App app{"Call paths to BDD"};

  std::vector<std::filesystem::path> input_call_path_files;
  std::filesystem::path input_bdd_file;
  std::filesystem::path output_bdd_file;

  app.add_option("call-paths", input_call_path_files, "Call paths");
  app.add_option("--in", input_bdd_file, "Input file for BDD deserialization.");
  app.add_option("--out", output_bdd_file, "Output file for BDD serialization.");

  CLI11_PARSE(app, argc, argv);

  if (input_bdd_file.empty() && input_call_path_files.empty()) {
    std::cout << "No input files provided.\n";
    return 1;
  }

  LibCore::SymbolManager manager;

  std::unique_ptr<LibBDD::BDD> bdd;
  if (input_bdd_file.empty()) {
    LibBDD::call_paths_t call_paths(input_call_path_files, &manager);
    bdd = std::make_unique<LibBDD::BDD>(call_paths.get_view());
  } else {
    bdd = std::make_unique<LibBDD::BDD>(input_bdd_file, &manager);
  }

  LibBDD::PrinterDebug printer;
  bdd->visit(printer);

  if (!output_bdd_file.empty()) {
    bdd->serialize(output_bdd_file);
  }

  std::cout << "BDD size: " << bdd->size() << "\n";

  return 0;
}
