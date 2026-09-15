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
  DS_ID action;                           // The action holding the original op.
  compute_op_t op;                        // The original op, as placed.
  std::unordered_set<DS_ID> pass_actions; // The compute actions of its own pass when it was placed.
};

// Sharing goes further than equal values: bf-p4c shares the units for the same hash of the same
// *field*, whatever value the field holds on each branch (hdu8.p4). So an op of the same shape
// as a placed one -- same function, operands equal but for one plain value -- can reuse it once
// that value reads one field on both paths. A value computed on both paths is unified by naming
// its two producers' outputs alike; a value one path reads from the packet, or as a constant, is
// moved into the other path's field on that path first (the ground truth's `msg3_sel`). The
// placed op's operand is then rewritten to the shared field, and the move is emitted at the
// start of the run holding the op it serves. One differing value, because a chain diverges in
// one input at a time, and a move per input to share one op is never worth it.
struct compute_move_t {
  DS_ID action;               // The keyless action holding the move.
  std::string op_id;          // The move's op id (the action's one op).
  klee::ref<klee::Expr> from; // The plain value read.
  klee::ref<klee::Expr> to;   // The shared symbol written.
  std::string anchor;         // The op whose path and run the move belongs to.
};

// An unrolled loop (Context::get_loops) computes the same body op in every iteration, on other
// values each time: no two iterations share a key above. An iteration placed in a later pass, or
// in another loop with the same body, runs where an earlier one ran instead, when both sit at the
// same offset of their pass in the same gress: the op then reads the fields that iteration read.
// The values the two read from the iteration before differ; each such pair of values is kept in
// one field (plan_value_homes), so the action reads the right one whichever pass calls it.
struct loop_key_t {
  size_t body;    // The first loop with this body (loops_t::body_of).
  size_t body_op; // The body op.
  bool egress;
  size_t offset; // The iteration's position among the loop's iterations in its pass.
  bool operator<(const loop_key_t &other) const {
    return std::tie(body, body_op, egress, offset) < std::tie(other.body, other.body_op, other.egress, other.offset);
  }
};

struct compute_reuse_state_t {
  std::map<loop_key_t, std::vector<compute_reuse_t>> by_loop_key;                         // Every iteration op placed at each key, the first first.
  std::vector<std::pair<klee::ref<klee::Expr>, klee::ref<klee::Expr>>> loop_field_shares; // Values one field holds (loop_key_t).
  std::unordered_map<std::string, size_t> loop_roles; // A loop state value -> the body op whose value it stands for.
  // Actions holding iteration ops, with the key of the first (its body op aside): an action takes
  // ops of one offset of one loop body only, so a later pass calls it whole.
  std::unordered_map<DS_ID, loop_key_t> loop_actions;
  std::map<compute_key_t, compute_reuse_t> by_key;              // Every op placed, by what it computes.
  std::map<compute_key_t, std::vector<compute_key_t>> by_shape; // The keys of every op of a shape (plain values blanked).
  std::unordered_map<std::string, compute_reuse_t> reused;      // By the id of the op that reused it.
  std::unordered_set<std::string> own_path_reuses;              // The ids in `reused` whose original is on their own path.
  std::unordered_map<std::string, const klee::Array *> aliases; // Output symbol of a reused op -> the original's.
  std::unordered_set<DS_ID> shared;                             // Actions reused by another path: they take no more ops.
  std::unordered_map<std::string, std::string> producers;       // Symbol -> the op that computes it.
  // Placed ops whose operands were rewritten to a shared field, by op id: (read before, read now).
  std::unordered_map<std::string, std::vector<std::pair<klee::ref<klee::Expr>, klee::ref<klee::Expr>>>> rewrites;
  std::vector<compute_move_t> moves;
  // Ops whose output symbol was unified with another path's: by op id, the symbol whose
  // variable the op writes instead of its own.
  std::unordered_map<std::string, klee::ref<klee::Expr>> output_aliases;
};

// What sharing an op with a placed one of the same shape takes.
struct compute_shape_match_t {
  compute_reuse_t original;
  std::vector<std::pair<klee::ref<klee::Expr>, klee::ref<klee::Expr>>> aliased; // (original's symbol, ours): unified by naming.
  std::vector<std::pair<klee::ref<klee::Expr>, klee::ref<klee::Expr>>>
      moved_there; // (original's plain value, our symbol): a move on the original's path.
  std::vector<std::pair<klee::ref<klee::Expr>, klee::ref<klee::Expr>>> moved_here; // (original's symbol, our plain value): a move on ours.
};

class TofinoContext : public TargetContext {
private:
  DataStructures data_structures;
  TNA tna;
  LibCore::Cow<compute_reuse_state_t> compute_reuse;

public:
  TofinoContext(const tna_config_t &tna_config) : data_structures(), tna(tna_config, data_structures), compute_reuse() {
    place_port_to_nf_dev_table();
  }
  TofinoContext(const TofinoContext &other)
      : data_structures(other.data_structures), tna(other.tna, data_structures), compute_reuse(other.compute_reuse) {}

  virtual TargetContext *clone() const override { return new TofinoContext(*this); }

  const TNA &get_tna() const { return tna; }
  TNA &get_mutable_tna() { return tna; }

  const DataStructures &get_data_structures() const { return data_structures; }
  DataStructures &get_mutable_data_structures() { return data_structures; }

  void parser_transition(const BDDNode *_node, klee::ref<klee::Expr> hdr, const BDDNode *last_parser_op, std::optional<bool> direction);
  void parser_select(const BDDNode *_node, const parser_select_t &select, const BDDNode *last_parser_op, std::optional<bool> direction);
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
  // `op` reuses `original`: its output symbol becomes another name for the original's (or the
  // original's name, when it computed the value without naming one), and the original's action
  // is shared from now on -- unless `same_path`: then the original is this very path's, and
  // the op merely names what it computed.
  void reuse_compute_op(const compute_op_t &op, const compute_reuse_t &original, bool same_path);
  // A (canonical) op placed in `action`, with the compute actions of its pass so far, for later
  // ops to find.
  void register_compute_op(const compute_op_t &op, DS_ID action, const std::unordered_set<DS_ID> &pass_actions);
  std::optional<compute_reuse_t> get_compute_reuse(const std::string &op_id) const;
  // Loop iterations sharing actions (loop_key_t).
  // Every op placed at `key`, the first first.
  std::vector<compute_reuse_t> find_loop_originals(const loop_key_t &key) const;
  void register_loop_op(const loop_key_t &key, const compute_op_t &op, DS_ID action);
  // `ours` and `theirs` are kept in one field.
  void share_loop_field(klee::ref<klee::Expr> ours, klee::ref<klee::Expr> theirs);
  const std::vector<std::pair<klee::ref<klee::Expr>, klee::ref<klee::Expr>>> &get_loop_field_shares() const {
    return compute_reuse->loop_field_shares;
  }
  std::optional<size_t> get_loop_role(const std::string &symbol) const;
  void set_loop_role(const std::string &symbol, size_t role);
  // `action` holds an iteration op placed at `key`, unless it held one already.
  void mark_loop_action(DS_ID action, const loop_key_t &key);
  // Whether `action` may take an op at `key` (an iteration op), or an op of no loop (null).
  bool takes_loop_op(DS_ID action, const loop_key_t *key) const;
  // Whether `op_id` reused an op of its own path (the value was computed there already).
  bool is_own_path_reuse(const std::string &op_id) const { return compute_reuse->own_path_reuses.contains(op_id); }
  bool is_shared_compute_action(DS_ID action) const { return compute_reuse->shared.contains(action); }

  // The placed ops of the same shape as the (canonical) `op`, on another path (their action not
  // in `path_actions`, this path's own), differing in one plain value that one of the two paths
  // computes in its current pass (`pass_actions` are ours), each with what unifying them takes;
  // fewest moves first, then earliest stage.
  std::vector<compute_shape_match_t> find_shape_matches(const compute_op_t &op, const std::unordered_set<DS_ID> &pass_actions,
                                                        const std::unordered_set<DS_ID> &path_actions) const;
  std::optional<klee::ref<klee::Expr>> get_output_alias(const std::string &op_id) const;
  // `symbol` (ours) becomes another name for `original_symbol`'s field.
  // Record that `symbol` and `original_symbol` name one value, and so one field. Only true when
  // the op producing `symbol` really runs in `shared_action`: either it is `placing`, the op this
  // share is about to put there, or it was shared into that action earlier. A match lines up the
  // operands of the ops it matches as well as their results, and those operands belong to ops of
  // their own, which this share does not move; claiming they share a field would leave the word
  // assignment looking for a value in a field its path never writes.
  void alias_symbol(klee::ref<klee::Expr> symbol, klee::ref<klee::Expr> original_symbol);
  // Whether that claim would be true: the op producing `symbol` has to run in `shared_action`,
  // either because it is `placing`, the op this share is about to put there, or because an
  // earlier share already moved it. A match lines up the operands of the ops it matches as well
  // as their results, and an operand belongs to an op of its own which this share does not move.
  bool can_alias_symbol(klee::ref<klee::Expr> symbol, const DS_ID &shared_action, const std::string &placing) const;
  // The placed op `op_id` reads `to` where it read `from`; its registry key follows.
  void rewrite_compute_op(const std::string &op_id, klee::ref<klee::Expr> from, klee::ref<klee::Expr> to);
  void add_compute_move(const compute_move_t &move);
  const std::vector<compute_move_t> &get_compute_moves() const { return compute_reuse->moves; }
  std::optional<compute_move_t> find_compute_move(klee::ref<klee::Expr> to) const;
  // The id of the placed op that computes `symbol` (a whole read of a symbol), if one was
  // registered: how the registry tells a value this plan computes from one it reads.
  std::optional<std::string> get_producer(klee::ref<klee::Expr> symbol) const;
  // `expr` as the placed op `op_id` reads it now.
  klee::ref<klee::Expr> apply_rewrites(const std::string &op_id, klee::ref<klee::Expr> expr) const;

  void debug() const override;
  // The pipeline's gress is where the active leaf is on its own path: past the last crossing
  // since the last recirculation it is the egress, otherwise the ingress. The flag the modules
  // set is the one the previous step's path left, which is another path's when the search moves
  // to another leaf.
  void sync_active_leaf(const EP *ep) override;
  void dump(std::ostream &os) const override { tna.pipeline.dump(os); }

private:
  void place_port_to_nf_dev_table();

public:
  // Every data structure placed since the last recirculation: the program-order dependencies
  // a stateful step must follow, with its control dependencies.
  static std::unordered_set<DS_ID> get_stateful_deps(const EP *ep, const BDDNode *node);
  // The producers of every condition guarding `node` in its pass, and the port-to-device table
  // for a condition on the device. A gateway that reads a table's result is a match dependency
  // on that table, so what it guards starts the stage after -- a table, or a compute action,
  // whose table bf-p4c fuses the gateway with. On data-flow dependencies alone SmartCookie's
  // cookie chain sat at stage 0, where bf-p4c has it at 4: behind the device gateway, the
  // bloom filter's rows and the gateway on their estimate. Same walk as get_dataflow_deps: with
  // speculations the pass's speculated steps first, then the plan's from the leaf of the node's
  // branch, until the pass's start.
  static std::unordered_set<DS_ID> get_control_deps(const EP *ep, const BDDNode *node, const speculations_t *speculations = nullptr);
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