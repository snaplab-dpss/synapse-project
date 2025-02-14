#include <LibBDD/BDD.h>

#include <filesystem>
#include <CLI/CLI.hpp>

int main(int argc, char **argv) {
  CLI::App app{"Inspect BDD"};

  std::filesystem::path input_bdd_file;

  app.add_option("--in", input_bdd_file, "Input file for BDD deserialization.")->required();

  CLI11_PARSE(app, argc, argv);

  std::cout << "Inspecting BDD " << input_bdd_file << "...\n";

  LibCore::SymbolManager manager;
  LibBDD::BDD bdd(input_bdd_file, &manager);
  LibBDD::BDD::inspection_report_t report = bdd.inspect();

  if (report.status == LibBDD::BDD::InspectionStatus::Ok) {
    std::cout << "Inspection report: PASSED\n";
  } else {
    std::cout << "Inspection report: FAILED\n";
    std::cout << report.message << ".\n";
  }

  return report.status == LibBDD::BDD::InspectionStatus::Ok ? 0 : 1;
}
