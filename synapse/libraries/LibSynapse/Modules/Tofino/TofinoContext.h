#pragma once

#include <LibSynapse/Context.h>
#include <LibSynapse/Modules/Tofino/TNA/TNA.h>
#include <LibSynapse/Modules/Tofino/DataStructures/DataStructures.h>

#include <unordered_map>
#include <optional>

namespace LibSynapse {
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
  bool can_place(const EP *ep, const BDDNode *node, const DS *ds) const;

  void debug() const override;

  static std::unordered_set<DS_ID> get_stateful_deps(const EP *ep, const BDDNode *node);
};

} // namespace Tofino

EXPLICIT_TARGET_CONTEXT_INSTANTIATION(Tofino, TofinoContext)

} // namespace LibSynapse