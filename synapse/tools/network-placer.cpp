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

  std::unordered_map<LibSynapse::TargetType, std::unique_ptr<const BDD>> target_bdds = placer.process();

  for (const auto &[target, target_bdd] : target_bdds) {

    if (show_bdd) {
      BDDViz::visualize(target_bdd.get(), false);
    }

    if (!output_bdd_file.empty()) {
      std::filesystem::path out_file(output_bdd_file.string() + "_device_" + std::to_string(target.instance_id) + ".bdd");
      target_bdd->serialize(out_file);
    }
  }

  return 0;
}
