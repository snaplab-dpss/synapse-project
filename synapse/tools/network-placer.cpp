#include <LibClone/Placer.h>

#include <LibBDD/BDD.h>
#include <LibBDD/Visitors/BDDVisualizer.h>

#include <LibCore/Debug.h>

#include <filesystem>
#include <CLI/CLI.hpp>

using namespace LibCore;
using namespace LibBDD;
using namespace LibSynapse;
using namespace LibClone;

// using LibBDD::BDDViz;

int main(int argc, char **argv) {
  CLI::App app{"Network placer"};

  std::filesystem::path input_bdd_file;
  std::filesystem::path input_physical_network_file;
  std::filesystem::path output_bdd_file;
  bool show_bdd{false};

  app.add_option("--in", input_bdd_file, "Input BDD file.")->required();
  app.add_option("--network", input_physical_network_file, "Input Physical Network file.")->required();
  app.add_option("--out", output_bdd_file, "Output file for the generated BDD.")->default_val(".");

  app.add_flag("--show-bdd", show_bdd, "Show the output BDD");
  CLI11_PARSE(app, argc, argv);

  SymbolManager symbol_manager;
  BDD bdd(input_bdd_file, &symbol_manager);

  const PhysicalNetwork phys_net = PhysicalNetwork::parse(input_physical_network_file);
  phys_net.debug();

  Placer placer = Placer(bdd, phys_net);

  std::unique_ptr<const BDD> new_bdd = placer.add_send_to_device_nodes();

  const BDD::inspection_report_t report = new_bdd->inspect();
  assert_or_panic(report.status == BDD::InspectionStatus::Ok, "BDD inspection failed: %s", report.message.c_str());
  std::cout << "BDD inspection passed.\n";

  if (show_bdd) {
    BDDViz::visualize(new_bdd.get(), false);
  }

  if (!output_bdd_file.empty()) {
    new_bdd->serialize(output_bdd_file);
  }

  return 0;
}
