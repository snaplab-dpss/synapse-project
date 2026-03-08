#include <LibClone/EmbeddingEngine.h>

#include <LibBDD/BDD.h>
#include <LibBDD/Visitors/BDDVisualizer.h>

#include <LibCore/Debug.h>

#include <filesystem>
#include <CLI/CLI.hpp>

using namespace LibCore;
using namespace LibBDD;
using namespace LibSynapse;
using namespace LibClone;

int main(int argc, char **argv) {
  CLI::App app{"Embedding engine"};

  std::filesystem::path input_bdd_file;
  std::filesystem::path input_physical_network_file;
  std::filesystem::path output_file;
  bool show_bdd{false};

  app.add_option("--in", input_bdd_file, "Input BDD file.")->required();
  app.add_option("--network", input_physical_network_file, "Input Physical Network file.")->required();
  app.add_option("--out", output_file, "Output file for the generated embedding.")->default_val(".");
  app.add_flag("--show-bdd", show_bdd, "Show the input BDD");

  CLI11_PARSE(app, argc, argv);

  SymbolManager symbol_manager;
  const BDD bdd(input_bdd_file, &symbol_manager);

  const PhysicalNetwork phys_net = PhysicalNetwork::parse(input_physical_network_file);

  EmbeddingEngine engine = EmbeddingEngine(bdd, phys_net);

  engine.pre_process();
  engine.debug();

  return 0;
}