#include <LibSynapse/Modules/Tofino/TNA/Pipeline.h>
#include <LibSynapse/Modules/Tofino/TNA/TNAProperties.h>
#include <LibSynapse/Modules/Tofino/DataStructures/DataStructures.h>
#include <LibSynapse/Target.h>

#include <LibSynapse/Modules/Tofino/TNA/SolverPlacer.h>

#include <filesystem>
#include <fstream>
#include <CLI/CLI.hpp>

using namespace LibSynapse;
using namespace LibSynapse::Tofino;

int main(int argc, char **argv) {
  CLI::App app{"simple-placer-solver"};

  std::filesystem::path targets_config_file;

  app.add_option("--config", targets_config_file, "Configuration file.")->required();

  CLI11_PARSE(app, argc, argv);

  const targets_config_t targets_config(targets_config_file);
  const tna_properties_t &tna_properties = targets_config.tofino_config.properties;

  std::cerr << "Active solver: " << SolverPlacer::get_active_solver() << "\n";

  DataStructures data_structures;
  Pipeline pipeline(tna_properties, data_structures);

  // HHTable *hhtable = new HHTable(tna_properties, "hhtable", 0, 65536, {32}, 1024, 4, 0);
  // SolverPlacer::find_placements(pipeline, hhtable, {});

  HHTable *hhtable = new HHTable(tna_properties, "hhtable", 0, 65536, {32}, 1024, 4, 0);
  pipeline.place(hhtable, {});
  data_structures.save(0, std::unique_ptr<DS>(hhtable));
  pipeline.debug();
  std::cerr << "HHTable placed with ID: " << hhtable->id << "\n";

  Table *t0 = new Table("t0", 65536, {32, 32, 16, 16}, {32});
  pipeline.place(t0, {hhtable->id});
  data_structures.save(0, std::unique_ptr<DS>(t0));
  pipeline.debug();
  std::cerr << "Table t0 placed with ID: " << t0->id << "\n";

  Table *t1 = new Table("t1", 65536, {32, 32, 16, 16}, {32});
  pipeline.place(t1, {hhtable->id});
  data_structures.save(1, std::unique_ptr<DS>(t1));
  pipeline.debug();
  std::cerr << "Table t1 placed with ID: " << t1->id << "\n";

  Table *t2 = new Table("t2", 65536, {32, 32, 16, 16}, {32});
  pipeline.place(t2, {t0->id, t1->id});
  data_structures.save(2, std::unique_ptr<DS>(t2));
  pipeline.debug();
  std::cerr << "Table t2 placed with ID: " << t2->id << "\n";

  return 0;
}