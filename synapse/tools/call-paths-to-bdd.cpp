#include <LibBDD/BDD.h>
#include <LibBDD/Unroll.h>
#include <LibBDD/Visitors/PrinterDebug.h>
#include <LibCore/Debug.h>

#include <fstream>
#include <filesystem>
#include <CLI/CLI.hpp>

using namespace LibCore;
using namespace LibBDD;

int main(int argc, char **argv) {
  CLI::App app{"Call paths to BDD"};

  std::vector<std::filesystem::path> input_call_path_files;
  std::filesystem::path input_bdd_file;
  std::filesystem::path output_bdd_file;
  bool no_unroll{false};
  bool no_group_loops{false};

  app.add_option("call-paths", input_call_path_files, "Call paths");
  app.add_option("--in", input_bdd_file, "Input file for BDD deserialization.");
  app.add_option("--out", output_bdd_file, "Output file for BDD serialization.");
  app.add_flag("--no-unroll", no_unroll, "Keep nested arithmetic in expressions instead of unrolling it into op_* nodes (see LibBDD/Unroll.h).");
  app.add_flag("--no-group-loops", no_group_loops, "Leave the iterations of unrolled loops interleaved (see BDD::group_loop_iterations).");

  CLI11_PARSE(app, argc, argv);

  if (input_bdd_file.empty() && input_call_path_files.empty()) {
    std::cout << "No input files provided.\n";
    return 1;
  }

  SymbolManager manager;

  std::unique_ptr<BDD> bdd;
  if (input_bdd_file.empty()) {
    call_paths_t call_paths(input_call_path_files, &manager);
    bdd = std::make_unique<BDD>(call_paths.get_view());
  } else {
    bdd = std::make_unique<BDD>(input_bdd_file, &manager);
  }

  if (!no_unroll) {
    unroll_arithmetic(*bdd);
  }

  if (!no_group_loops) {
    const std::vector<loop_t> loops = bdd->detect_loops();
    bdd->group_loop_iterations(loops);
    std::cout << "Grouped the iterations of " << loops.size() << " loop(s).\n";
  }

  const BDD::inspection_report_t bdd_inspection_report = bdd->inspect();
  if (bdd_inspection_report.status != BDD::InspectionStatus::Ok) {
    panic("BDD inspection failed: %s", bdd_inspection_report.message.c_str());
  } else {
    std::cout << "BDD inspection succeeded.\n";
  }

  if (!output_bdd_file.empty()) {
    bdd->serialize(output_bdd_file);
  }

  std::cout << "BDD size: " << bdd->size() << "\n";

  return 0;
}
