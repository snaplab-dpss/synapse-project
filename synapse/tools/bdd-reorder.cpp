#include <filesystem>
#include <CLI/CLI.hpp>

#include "../src/bdd/bdd.h"
#include "../src/exprs/exprs.h"

void print(const BDD *bdd, const reorder_op_t &op) {
  const anchor_info_t &anchor_info = op.anchor_info;
  const candidate_info_t &candidate_info = op.candidate_info;

  const Node *anchor = bdd->get_node_by_id(anchor_info.id);
  const Node *evicted = bdd->get_node_by_id(op.evicted_id);
  const Node *candidate = bdd->get_node_by_id(candidate_info.id);

  assert(anchor && "Anchor node not found");
  assert(candidate && "Proposed candidate not found");

  std::cerr << "\n==================================\n";

  std::cerr << "* Anchor:\n";
  std::cerr << "\t" << anchor->dump(true) << " -> " << anchor_info.direction
            << "\n";
  std::cerr << "* Evicted:\n";
  std::cerr << "\t" << evicted->dump(true) << "\n";
  std::cerr << "* Candidate:\n";
  std::cerr << "\t" << candidate->dump(true) << "\n";

  if (candidate_info.siblings.size() > 0) {
    std::cerr << "* Siblings: ";
    for (node_id_t sibling : candidate_info.siblings) {
      std::cerr << sibling << " ";
    }
    std::cerr << "\n";
  }

  if (!candidate_info.condition.isNull()) {
    std::cerr << "* Condition: "
              << expr_to_string(candidate_info.condition, true) << "\n";
  }

  std::cerr << "==================================\n";
}

void list_candidates(const BDD *bdd, const anchor_info_t &anchor_info) {
  std::vector<reorder_op_t> ops = get_reorder_ops(bdd, anchor_info, false);

  std::cerr << "Available reordering operations: " << ops.size() << "\n";
  for (const reorder_op_t &op : ops) {
    print(bdd, op);
  }
}

void apply_reordering_ops(
    const BDD *bdd,
    const std::vector<std::pair<anchor_info_t, node_id_t>> &ops) {
  for (const std::pair<anchor_info_t, node_id_t> &op : ops) {
    anchor_info_t anchor_info = op.first;
    node_id_t candidate_id = op.second;

    std::cerr << "-> Reordering op:";
    std::cerr << " anchor=" << anchor_info.id;
    std::cerr << " candidate=" << candidate_id;
    std::cerr << "\n";

    reordered_bdd_t reordered_bdd = try_reorder(bdd, anchor_info, candidate_id);

    if (reordered_bdd.op.candidate_info.status !=
        ReorderingCandidateStatus::Valid) {
      std::cerr << "Reordering failed: "
                << reordered_bdd.op.candidate_info.status << "\n";
      break;
    } else {
      assert(reordered_bdd.bdd);
      BDDViz::visualize(reordered_bdd.bdd.get(), true);
    }
  }
}

void apply_all_candidates(const BDD *bdd, node_id_t anchor_id) {
  auto start = std::chrono::steady_clock::now();

  std::vector<reordered_bdd_t> bdds = reorder(bdd, anchor_id);

  auto end = std::chrono::steady_clock::now();
  auto elapsed = end - start;
  auto elapsed_seconds =
      std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

  std::cerr << "Total: " << bdds.size() << "\n";
  std::cerr << "Elapsed: " << elapsed_seconds << " seconds\n";

  for (const reordered_bdd_t &reordered_bdd : bdds) {
    std::cerr << "\n==================================\n";
    std::cerr << "Candidate: " << reordered_bdd.op.candidate_info.id << "\n";
    if (reordered_bdd.op2.has_value()) {
      std::cerr << "Candidate2: " << reordered_bdd.op2->candidate_info.id
                << "\n";
    }
    std::cerr << "==================================\n";

    // BDDViz::visualize(reordered_bdd.bdd, true);
  }
}

void estimate(const BDD *bdd) {
  auto start = std::chrono::steady_clock::now();

  double approximation = estimate_reorder(bdd);

  auto end = std::chrono::steady_clock::now();
  auto elapsed = end - start;
  auto elapsed_seconds =
      std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

  std::cerr << "Approximately " << approximation << " BDDs generated\n";
  std::cerr << "Elapsed: " << elapsed_seconds << " seconds\n";
}

int main(int argc, char **argv) {
  CLI::App app{"BDD reorder"};

  std::filesystem::path input_bdd_file;

  app.add_option("--in", input_bdd_file, "Input file for BDD deserialization.")
      ->required();

  CLI11_PARSE(app, argc, argv);

  std::unique_ptr<BDD> bdd = std::make_unique<BDD>(input_bdd_file);

  // list_candidates(bdd, {20, true});
  apply_reordering_ops(bdd.get(), {
                                      {{2, false}, 8},
                                  });
  // test_reorder(bdd, 3);
  // estimate(bdd);

  return 0;
}
