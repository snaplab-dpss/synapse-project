#pragma once

#include <LibBDD/BDD.h>

#include <optional>
#include <memory>

namespace LibBDD {

enum class ReorderingCandidateStatus {
  Valid,
  UnreachableCandidate,
  CandidateFollowsAnchor,
  IOCheckFailed,
  RWCheckFailed,
  NotAllowed,
  ConflictingRouting,
  ImpossibleCondition,
};

std::ostream &operator<<(std::ostream &os, const ReorderingCandidateStatus &status);

struct candidate_info_t {
  node_id_t id;
  bool is_branch;
  std::unordered_set<node_id_t> siblings;
  klee::ref<klee::Expr> condition;
  ReorderingCandidateStatus status;
};

struct anchor_info_t {
  node_id_t id;
  bool direction; // When the anchor is a branch, this field indicates the
                  // direction of the branch (true or false). The reordering
                  // operation will respect this direction.

  std::string to_string() const {
    std::stringstream ss;
    ss << "anchor{id=" << id << ",direction=" << direction << "}";
    return ss.str();
  }
};

struct reorder_op_t {
  anchor_info_t anchor_info;
  node_id_t evicted_id;
  candidate_info_t candidate_info;

  std::string to_string() const {
    std::stringstream ss;
    ss << "reorder{";
    ss << anchor_info.to_string();
    ss << ",evicted=" << evicted_id;
    ss << ",candidate=" << candidate_info.id;
    ss << "}";
    return ss.str();
  }
};

struct translated_symbol_t {
  LibCore::symbol_t old_symbol;
  LibCore::symbol_t new_symbol;
};

struct reordered_bdd_t {
  std::unique_ptr<BDD> bdd;
  reorder_op_t op;

  // When the anchor is a branch, this field may contain the second reordering
  // operation that was applied to the branch.
  std::optional<reorder_op_t> op2;

  std::vector<translated_symbol_t> translated_symbols;
};

std::vector<reordered_bdd_t> reorder(const BDD *bdd, node_id_t anchor_id, bool allow_shape_altering_ops = true);
std::vector<reordered_bdd_t> reorder(const BDD *bdd, const anchor_info_t &anchor_info, bool allow_shape_altering_ops = true);
reordered_bdd_t try_reorder(const BDD *bdd, const anchor_info_t &anchor_info, node_id_t candidate_id);
std::vector<reorder_op_t> get_reorder_ops(const BDD *bdd, const anchor_info_t &anchor_info, bool allow_shape_altering_ops = true);
std::unique_ptr<BDD> reorder(const BDD *bdd, const reorder_op_t &op, std::vector<translated_symbol_t> &translated_symbols);
double estimate_reorder(const BDD *bdd);

} // namespace LibBDD