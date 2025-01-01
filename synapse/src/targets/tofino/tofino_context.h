#pragma once

#include <unordered_map>
#include <optional>
#include <toml++/toml.hpp>

#include "tna/tna.h"
#include "data_structures/data_structures.h"
#include "../../execution_plan/context.h"

namespace tofino {

class TofinoContext : public TargetContext {
private:
  TNA tna;
  std::unordered_map<addr_t, std::unordered_set<DS *>> obj_to_ds;
  std::unordered_map<DS_ID, DS *> id_to_ds;

public:
  TofinoContext(const toml::table &config);
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

  void parser_transition(const EP *ep, const Node *node, klee::ref<klee::Expr> hdr);

  void parser_select(const EP *ep, const Node *node, klee::ref<klee::Expr> field,
                     const std::vector<int> &values, bool negate);

  void parser_accept(const EP *ep, const Node *node);
  void parser_reject(const EP *ep, const Node *node);

  std::unordered_set<DS_ID> get_stateful_deps(const EP *ep, const Node *node) const;

  void place(EP *ep, const Node *node, addr_t obj, DS *ds);

  bool check_placement(const EP *ep, const Node *node, const DS *ds) const;
  bool check_many_placements(const EP *ep, const Node *node,
                             const std::vector<std::unordered_set<DS *>> &ds) const;

  void debug() const override;
};

} // namespace tofino

EXPLICIT_TARGET_CONTEXT_INSTANTIATION(tofino, TofinoContext)
