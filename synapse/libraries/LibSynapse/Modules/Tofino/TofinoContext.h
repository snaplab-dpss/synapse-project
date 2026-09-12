#pragma once

#include <LibSynapse/Context.h>
#include <LibSynapse/Modules/Tofino/TNA/TNA.h>
#include <LibSynapse/Modules/Tofino/DataStructures/DataStructures.h>

#include <unordered_map>
#include <optional>

namespace LibSynapse {
namespace Tofino {} // namespace Tofino
struct speculations_t;
namespace Tofino {

class TofinoContext : public TargetContext {
private:
  DataStructures data_structures;
  TNA tna;

public:
  TofinoContext(const tna_config_t &tna_config) : data_structures(), tna(tna_config, data_structures) {}
  TofinoContext(const TofinoContext &other) : data_structures(other.data_structures), tna(other.tna, data_structures) {}

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
  // The compute action holding the op with this id.
  DS_ID find_compute_action(const std::string &op_id) const;

  void debug() const override;

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