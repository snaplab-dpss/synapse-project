#include <LibBDD/BDD.h>
#include <LibBDD/Reorder.h>
#include <LibBDD/Visitors/BDDVisualizer.h>
#include <LibBDD/Visitors/BDDProfileVisualizer.h>
#include <LibCore/Expr.h>

#include <filesystem>
#include <optional>
#include <CLI/CLI.hpp>

using namespace LibCore;
using namespace LibBDD;

void print(const BDD *bdd, const reorder_op_t &op) {
  const anchor_info_t &anchor_info       = op.anchor_info;
  const candidate_info_t &candidate_info = op.candidate_info;

  const BDDNode *anchor    = bdd->get_node_by_id(anchor_info.id);
  const BDDNode *evicted   = bdd->get_node_by_id(op.evicted_id);
  const BDDNode *candidate = bdd->get_node_by_id(candidate_info.id);

  assert(anchor && "Anchor node not found");
  assert(candidate && "Proposed candidate not found");

  std::cerr << "\n==================================\n";

  std::cerr << "* Anchor:\n";
  std::cerr << "\t" << anchor->dump(true) << " -> " << anchor_info.direction << "\n";
  std::cerr << "* Evicted:\n";
  std::cerr << "\t" << evicted->dump(true) << "\n";
  std::cerr << "* Candidate:\n";
  std::cerr << "\t" << candidate->dump(true) << "\n";

  if (candidate_info.siblings.size() > 0) {
    std::cerr << "* Siblings: ";
    for (bdd_node_id_t sibling : candidate_info.siblings) {
      std::cerr << sibling << " ";
    }
    std::cerr << "\n";
  }

  if (!candidate_info.condition.isNull()) {
    std::cerr << "* Condition: " << expr_to_string(candidate_info.condition, true) << "\n";
  }

  std::cerr << "==================================\n";
}

void list_candidates(const BDD *bdd, const anchor_info_t &anchor_info) {
  const std::vector<reorder_op_t> ops = get_reorder_ops(bdd, anchor_info, false);

  std::cerr << "Available reordering operations: " << ops.size() << "\n";
  for (const reorder_op_t &op : ops) {
    print(bdd, op);
  }
}

void apply_reordering_ops(const BDD *bdd, const std::vector<std::pair<anchor_info_t, bdd_node_id_t>> &ops) {
  reordered_bdd_t reordered_bdd;

  for (const std::pair<anchor_info_t, bdd_node_id_t> &op : ops) {
    const anchor_info_t anchor_info  = op.first;
    const bdd_node_id_t candidate_id = op.second;

    std::cerr << "-> Reordering op:";
    std::cerr << " anchor=" << anchor_info.id;
    std::cerr << " candidate=" << candidate_id;
    std::cerr << "\n";

    reordered_bdd = try_reorder(bdd, anchor_info, candidate_id);

    if (reordered_bdd.op.candidate_info.status != ReorderingCandidateStatus::Valid) {
      std::cerr << "Reordering failed: " << reordered_bdd.op.candidate_info.status << "\n";
      break;
    } else {
      assert(reordered_bdd.bdd);
      BDDViz::visualize(reordered_bdd.bdd.get(), true);
    }

    bdd = reordered_bdd.bdd.get();
  }
}

void apply_all_candidates(const BDD *bdd, bdd_node_id_t anchor_id) {
  auto start = std::chrono::steady_clock::now();

  std::vector<reordered_bdd_t> bdds = reorder(bdd, anchor_id);

  auto end             = std::chrono::steady_clock::now();
  auto elapsed         = end - start;
  auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

  std::cerr << "Total: " << bdds.size() << "\n";
  std::cerr << "Elapsed: " << elapsed_seconds << " seconds\n";

  for (const reordered_bdd_t &reordered_bdd : bdds) {
    std::cerr << "\n==================================\n";
    std::cerr << "Candidate: " << reordered_bdd.op.candidate_info.id << "\n";
    if (reordered_bdd.op2.has_value()) {
      std::cerr << "Candidate2: " << reordered_bdd.op2->candidate_info.id << "\n";
    }
    std::cerr << "==================================\n";

    // BDDViz::visualize(reordered_bdd.bdd, true);
  }
}

void estimate(const BDD *bdd) {
  auto start = std::chrono::steady_clock::now();

  double approximation = estimate_reorder(bdd);

  auto end             = std::chrono::steady_clock::now();
  auto elapsed         = end - start;
  auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

  std::cerr << "Approximately " << approximation << " BDDs generated\n";
  std::cerr << "Elapsed: " << elapsed_seconds << " seconds\n";
}

int main(int argc, char **argv) {
  CLI::App app{"BDD reorder"};

  std::filesystem::path input_bdd_file;

  app.add_option("--in", input_bdd_file, "Input file for BDD deserialization.")->required();

  // The anchor is the node the candidate is moved to sit right after; for a branch anchor,
  // --direction picks the side. With --candidate the single op is attempted and its status is
  // reported, which says *why* when it is refused; without it every candidate is listed.
  bdd_node_id_t anchor_id = 0;
  bool direction          = true;
  std::vector<bdd_node_id_t> candidate_ids;

  app.add_option("--anchor", anchor_id, "Anchor node id.")->required();
  app.add_option("--direction", direction, "Branch direction at the anchor (default true).");
  app.add_option("--candidate", candidate_ids,
                 "Move this node to right after the anchor. Given more than once, each further candidate is moved to "
                 "right after the previous one, on the BDD the previous move produced.");

  CLI11_PARSE(app, argc, argv);

  SymbolManager symbol_manager;
  BDD bdd(input_bdd_file, &symbol_manager);

  anchor_info_t anchor_info{anchor_id, direction};

  if (!candidate_ids.empty()) {
    std::unique_ptr<BDD> current;
    const BDD *view = &bdd;
    for (bdd_node_id_t candidate_id : candidate_ids) {
      reordered_bdd_t result = try_reorder(view, anchor_info, candidate_id);
      std::cerr << "anchor=" << anchor_info.id << " direction=" << anchor_info.direction << " candidate=" << candidate_id
                << " -> " << result.op.candidate_info.status << "\n";
      if (result.op.candidate_info.status != ReorderingCandidateStatus::Valid) {
        return 1;
      }
      current     = std::move(result.bdd);
      view        = current.get();
      anchor_info = {candidate_id, true}; // the next candidate goes right after this one
    }
    return 0;
  }

  list_candidates(&bdd, anchor_info);

  return 0;
}
