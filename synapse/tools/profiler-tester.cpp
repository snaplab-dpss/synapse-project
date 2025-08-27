#include <LibCore/RandomEngine.h>
#include <LibCore/kQuery.h>
#include <LibCore/Debug.h>
#include <LibBDD/BDD.h>
#include <LibSynapse/Profiler.h>

#include <filesystem>
#include <fstream>
#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>

using namespace LibCore;
using namespace LibBDD;
using namespace LibSynapse;

const Branch *get_random_branch(const BDD &bdd) {
  if (!bdd.get_root()) {
    return nullptr;
  }

  const Branch *selected = nullptr;
  bool first             = true;

  bdd.get_root()->visit_nodes([&selected, &first](const BDDNode *node) {
    if (node->get_type() == BDDNodeType::Branch) {
      const Branch *branch = dynamic_cast<const Branch *>(node);
      if (!first && SingletonRandomEngine::generate() % 2 == 0) {
        selected = branch;
        return BDDNodeVisitAction::Stop;
      }
      first = false;
    }
    return BDDNodeVisitAction::Continue;
  });

  return selected;
}

int main(int argc, char **argv) {
  CLI::App app{"Synapse"};

  std::filesystem::path input_bdd_file;

  app.add_option("--in", input_bdd_file, "Input file for BDD deserialization.")->required();

  CLI11_PARSE(app, argc, argv);

  SingletonRandomEngine::seed(0);

  SymbolManager symbol_manager;
  const BDD bdd(input_bdd_file, &symbol_manager);
  const bdd_profile_t bdd_profile = build_uniform_bdd_profile(&bdd);

  Profiler profiler = Profiler(&bdd, bdd_profile, true);
  profiler.debug();

  const std::string kQueryStr = "array packet_chunks[1280] : w32 -> w8 = symbolic\n"
                                "(query [] false [(Eq w16 (ReadLSB w16 (w32 514) packet_chunks) (w16 25))])\n";

  kQueryParser kQueryParser(&symbol_manager);
  kQuery_t kQuery = kQueryParser.parse(kQueryStr);

  const Branch *random_branch = get_random_branch(bdd);
  if (!random_branch) {
    std::cout << "No random branch selected\n";
    return 1;
  }

  std::cout << "Selected branch: " << random_branch->dump(true) << "\n";

  // klee::ref<klee::Expr> expr = kQuery.values[0];
  // std::cout << "expr: " << LibCore::expr_to_string(expr, true) << "\n";

  const std::vector<klee::ref<klee::Expr>> constraints = random_branch->get_ordered_branch_constraints();
  for (auto c : constraints) {
    std::cout << "  * " << LibCore::pretty_print_expr(c, true) << "\n";
  }

  profiler.remove(constraints);
  // profiler.insert_relative(constraints, expr, 0.1_hr);
  profiler.debug();

  return 0;
}
