#include <LibClone/Network.h>
#include <LibBDD/BDD.h>

#include <filesystem>
#include <CLI/CLI.hpp>

using namespace LibCore;
using namespace LibBDD;
using namespace LibClone;

int main(int argc, char **argv) {
  CLI::App app{"Network consolidator"};

  std::filesystem::path input_network_consolidation_plan_file;
  std::filesystem::path output_consolidated_bdd_file;
  bool show_plan   = false;
  bool consolidate = false;

  app.add_option("--plan", input_network_consolidation_plan_file, "Plan for network consolidation.")->required();
  app.add_flag("--show-plan", show_plan, "Show the network consolidation plan.");
  app.add_flag("--consolidate", consolidate, "Consolidate the network.");
  app.add_option("--out", output_consolidated_bdd_file, "Output file for the consolidated BDD.");

  CLI11_PARSE(app, argc, argv);

  SymbolManager manager;
  Network network = Network::parse(input_network_consolidation_plan_file, &manager);
  network.debug();

  if (show_plan) {
    ClusterViz clusterviz = network.build_clusterviz();
    clusterviz.show(false);
    std::cout << "Network consolidation plan graph written to " << clusterviz.get_file_path().string() << std::endl;
  }

  if (consolidate) {
    const BDD bdd                         = network.consolidate();
    const BDD::inspection_report_t report = bdd.inspect();
    assert_or_panic(report.status == BDD::InspectionStatus::Ok, "BDD inspection failed: %s", report.message.c_str());
    std::cout << "BDD inspection passed.\n";
    BDDViz::visualize(&bdd, false);
    if (!output_consolidated_bdd_file.empty()) {
      bdd.serialize(output_consolidated_bdd_file);
    }
  }

  return 0;
}
