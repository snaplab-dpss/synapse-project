#include <LibBDD/BDD.h>
#include <LibBDD/Visitors/PrinterDebug.h>

#include <fstream>
#include <filesystem>
#include <CLI/CLI.hpp>

void assert_bdd(const LibBDD::BDD &bdd) {
  std::cout << "Asserting BDD...\n";

  const LibBDD::Node *root = bdd.get_root();
  assert(root);

  std::vector<const LibBDD::Node *> nodes{root};

  while (nodes.size()) {
    const LibBDD::Node *node = nodes[0];
    nodes.erase(nodes.begin());

    if (node->get_type() == LibBDD::NodeType::Branch) {
      const LibBDD::Branch *branch = static_cast<const LibBDD::Branch *>(node);

      const LibBDD::Node *on_true  = branch->get_on_true();
      const LibBDD::Node *on_false = branch->get_on_false();

      assert(on_true);
      assert(on_false);

      const LibBDD::Node *on_true_prev  = on_true->get_prev();
      const LibBDD::Node *on_false_prev = on_false->get_prev();

      assert(on_true_prev);
      assert(on_false_prev);

      assert(on_true_prev->get_id() == node->get_id());
      assert(on_false_prev->get_id() == node->get_id());

      nodes.push_back(on_true);
      nodes.push_back(on_false);
    } else {
      const LibBDD::Node *next = node->get_next();

      if (!next)
        continue;

      const LibBDD::Node *next_prev = next->get_prev();

      assert(next_prev);
      assert(next_prev->get_id() == node->get_id());

      nodes.push_back(next);
    }
  }

  std::cout << "OK!\n";
}

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
  assert_bdd(*bdd);

  LibBDD::PrinterDebug printer;
  bdd->visit(printer);

  if (!output_bdd_file.empty()) {
    bdd->serialize(output_bdd_file);
  }

  std::cout << "BDD size: " << bdd->size() << "\n";

  return 0;
}
