#pragma once

#include <LibSynapse/Context.h>
#include <LibSynapse/Modules/Tofino/TNA/TNA.h>
#include <LibSynapse/Modules/Tofino/DataStructures/DataStructures.h>

#include <LibCore/Cow.h>

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace LibSynapse {
namespace Tofino {} // namespace Tofino
struct speculations_t;
namespace Tofino {

// Two mutually exclusive paths of the BDD can carry the same stateless computation: SmartCookie
// hashes the same packet bytes on its SYN path and on its cookie-check path. Placed twice, the
// second copy asks the pipeline for its hash-distribution units a second time, and bf-p4c shares
// those between tables on exclusive branches only when they hash the same field
// (tofino/exp-compute/hdu*.p4). So an op that computes the same function of the same values as
// one already placed reuses that op's action, and the emitter writes its output symbol as the
// original's: one action, called from both branches, on one set of fields.
struct compute_key_t {
  std::string fn;
  std::vector<klee::ref<klee::Expr>> args;
  bool operator<(const compute_key_t &other) const;
};

struct compute_reuse_t {
  DS_ID action;    // The action holding the original op.
  compute_op_t op; // The original op, as placed.
};

struct compute_reuse_state_t {
  std::map<compute_key_t, compute_reuse_t> by_key;              // Every op placed, by what it computes.
  std::unordered_map<std::string, compute_reuse_t> reused;      // By the id of the op that reused it.
  std::unordered_map<std::string, const klee::Array *> aliases; // Output symbol of a reused op -> the original's.
  std::unordered_set<DS_ID> shared;                             // Actions reused by another path: they take no more ops.
};

class TofinoContext : public TargetContext {
private:
  DataStructures data_structures;
  TNA tna;
  LibCore::Cow<compute_reuse_state_t> compute_reuse;

public:
  TofinoContext(const tna_config_t &tna_config) : data_structures(), tna(tna_config, data_structures), compute_reuse() {}
  TofinoContext(const TofinoContext &other)
      : data_structures(other.data_structures), tna(other.tna, data_structures), compute_reuse(other.compute_reuse) {}

  virtual TargetContext *clone() const override { return new TofinoContext(*this); }

  const TNA &get_tna() const { return tna; }
  TNA &get_mutable_tna() { return tna; }

  const DataStructures &get_data_structures() const { return data_structures; }
  DataStructures &get_mutable_data_structures() { return data_structures; }

  void parser_transition(const BDDNode *_node, klee::ref<klee::Expr> hdr, const BDDNode *last_parser_op, std::optional<bool> direction);
  void parser_select(const BDDNode *_node, const std::vector<parser_selection_t> &selections, const BDDNode *last_parser_op,
                     std::optional<bool> direction);
  void parser_accept(const BDDNode *_node, const BDDNode *last_parser_op, std::optional<bool> direction);
  void parser_reject(const BDDNode *_node, const BDDNode *last_parser_op, std::optional<bool> direction);

  void place(EP *ep, const BDDNode *node, addr_t obj, DS *ds);
  void place(EP *ep, const BDDNode *node, addr_t obj, DS *ds, const std::unordered_set<DS_ID> &deps);
  void place(addr_t obj, DS *ds, const std::unordered_set<DS_ID> &deps);
  bool can_place(const EP *ep, const BDDNode *node, const DS *ds) const;
  bool can_place(const EP *ep, const BDDNode *node, const DS *ds, const std::unordered_set<DS_ID> &deps) const;
  bool can_place(const DS *ds, const std::unordered_set<DS_ID> &deps) const;
  // Simple placer only: for speculation, which cannot afford the ILP fallback per node.
  bool can_place_fast(const DS *ds, const std::unordered_set<DS_ID> &deps) const;

  struct compute_op_plan_t {
    DS_ID action_id;
    bool append; // Into an existing action of the run; otherwise a new action named action_id.
  };

  // Where a stateless op (needing `deps`) goes: appended to the earliest action of the
  // current run of compute steps that sits at or after all its producers and can still take
  // it, unless a new action could be placed in an earlier stage. Empty when neither fits.
  std::optional<compute_op_plan_t> plan_compute_op(const std::vector<DS_ID> &run_actions, DS_ID new_action_id, const compute_op_t &op,
                                                   const std::unordered_set<DS_ID> &deps) const;
  void append_compute_op(DS_ID action_id, const compute_op_t &op, const std::unordered_set<DS_ID> &deps);
  // The compute action holding the op with this id (the original's, for an op that reused one).
  DS_ID find_compute_action(const std::string &op_id) const;

  // Reuse of compute ops across mutually exclusive paths (compute_reuse_state_t above).
  // `value` with every symbol a reused op produced replaced by the original's.
  klee::ref<klee::Expr> canonical_value(klee::ref<klee::Expr> value) const;
  // `op` with its operands canonical: the form the registry keys on and the actions hold.
  compute_op_t canonical_op(const compute_op_t &op) const;
  // An op already placed that computes exactly what the (canonical) `op` computes.
  std::optional<compute_reuse_t> find_reusable_compute_op(const compute_op_t &op) const;
  // `op` reuses `original`: its output symbol becomes another name for the original's, and the
  // original's action is shared from now on.
  void reuse_compute_op(const compute_op_t &op, const compute_reuse_t &original);
  // A (canonical) op placed in `action`, for later ops to find.
  void register_compute_op(const compute_op_t &op, DS_ID action);
  std::optional<compute_reuse_t> get_compute_reuse(const std::string &op_id) const;
  bool is_shared_compute_action(DS_ID action) const { return compute_reuse->shared.contains(action); }

  void debug() const override;
  // The pipeline's gress is where the active leaf is on its own path: past the last crossing
  // since the last recirculation it is the egress, otherwise the ingress. The flag the modules
  // set is the one the previous step's path left, which is another path's when the search moves
  // to another leaf.
  void sync_active_leaf(const EP *ep) override;

  // Every data structure placed since the last recirculation: the program-order dependencies
  // a stateful step must follow.
  static std::unordered_set<DS_ID> get_stateful_deps(const EP *ep, const BDDNode *node);
  // Only the data structures that produce the symbols `value` reads (since the last
  // recirculation): what a stateless computation of `value` actually has to wait for, so
  // independent steps can share a stage.
  static std::unordered_set<DS_ID> get_dataflow_deps(const EP *ep, const BDDNode *node, klee::ref<klee::Expr> value);
  // Same, during speculation: producers speculated in the current pass count too, and the
  // plan's own producers no longer do once a recirculation was speculated.
  static std::unordered_set<DS_ID> get_dataflow_deps(const EP *ep, const BDDNode *node, klee::ref<klee::Expr> value,
                                                     const speculations_t &speculations);
};

} // namespace Tofino

EXPLICIT_TARGET_CONTEXT_INSTANTIATION(Tofino, TofinoContext)

} // namespace LibSynapse