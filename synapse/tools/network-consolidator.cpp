#include <LibClone/Network.h>
#include <LibBDD/BDD.h>

#include <filesystem>
#include <CLI/CLI.hpp>

int main(int argc, char **argv) {
  CLI::App app{"Network consolidator"};

  std::filesystem::path input_network_consolidation_plan_file;

  app.add_option("--plan", input_network_consolidation_plan_file, "Plan for network consolidation.")->required();

  CLI11_PARSE(app, argc, argv);

  LibCore::SymbolManager manager;
  LibClone::Network network = LibClone::Network::parse(input_network_consolidation_plan_file, &manager);

  network.debug();

  return 0;
}
