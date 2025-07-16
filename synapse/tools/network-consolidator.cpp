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

  app.add_option("--plan", input_network_consolidation_plan_file, "Plan for network consolidation.")->required();

  CLI11_PARSE(app, argc, argv);

  SymbolManager manager;
  Network network = Network::parse(input_network_consolidation_plan_file, &manager);
  network.debug();

  ClusterViz clusterviz = network.build_clusterviz();
  clusterviz.show(false);
  std::cout << "Network consolidation plan graph written to: " << clusterviz.get_file_path() << std::endl;

  const BDD bdd = network.consolidate();

  const BDD::inspection_report_t report = bdd.inspect();
  if (report.status != BDD::InspectionStatus::Ok) {
    std::cout << "BDD inspection failed: " << report.message << "\n";
    return 1;
  }

  std::cout << "BDD inspection passed.\n";
  BDDViz::visualize(&bdd, false);

  return 0;
}
