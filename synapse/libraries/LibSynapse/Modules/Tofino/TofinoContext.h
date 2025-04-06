#pragma once

#include <LibSynapse/Context.h>
#include <LibSynapse/Modules/Tofino/TNA/TNA.h>
#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>

#include <unordered_map>
#include <optional>

namespace LibSynapse {
namespace Tofino {

class TofinoContext : public TargetContext {
private:
  TNA tna;
  std::unordered_map<addr_t, std::unordered_set<DS *>> obj_to_ds;
  std::unordered_map<DS_ID, DS *> id_to_ds;

public:
  TofinoContext(const tna_config_t &tna_config);
  TofinoContext(const TofinoContext &other);

  ~TofinoContext();

  virtual TargetContext *clone() const override { return new TofinoContext(*this); }

  const TNA &get_tna() const { return tna; }
  TNA &get_mutable_tna() { return tna; }

  bool has_ds(addr_t addr) const;
  bool has_ds(DS_ID id) const;
  const std::unordered_set<DS *> &get_ds(addr_t addr) const;
  const DS *get_ds_from_id(DS_ID id) const;
  void save_ds(addr_t addr, DS *ds);

  template <class DS_T> const DS_T *get_single_ds(addr_t addr) const {
    const std::unordered_set<DS *> &ds = get_ds(addr);
    assert(ds.size() == 1 && "Expected exactly one DS");
    return dynamic_cast<const DS_T *>(*ds.begin());
  }

  void parser_transition(const LibBDD::Node *node, klee::ref<klee::Expr> hdr, const LibBDD::Node *last_parser_op, std::optional<bool> direction);
  void parser_select(const LibBDD::Node *node, const std::vector<parser_selection_t> &selections, const LibBDD::Node *last_parser_op,
                     std::optional<bool> direction);
  void parser_accept(const LibBDD::Node *node, const LibBDD::Node *last_parser_op, std::optional<bool> direction);
  void parser_reject(const LibBDD::Node *node, const LibBDD::Node *last_parser_op, std::optional<bool> direction);

  std::unordered_set<DS_ID> get_stateful_deps(const EP *ep, const LibBDD::Node *node) const;

  void place(EP *ep, const LibBDD::Node *node, addr_t obj, DS *ds);

  bool check_placement(const EP *ep, const LibBDD::Node *node, const DS *ds) const;
  bool check_many_placements(const EP *ep, const LibBDD::Node *node, const std::vector<std::unordered_set<DS *>> &ds) const;

  void debug() const override;
};

} // namespace Tofino

EXPLICIT_TARGET_CONTEXT_INSTANTIATION(Tofino, TofinoContext)

} // namespace LibSynapse