#pragma once

#include <LibBDD/Config.h>
#include <LibCore/Types.h>
#include <LibCore/SymbolManager.h>
#include <LibCore/Expr.h>

#include <functional>
#include <iostream>
#include <unordered_set>
#include <vector>

#include <klee/Constraints.h>

namespace LibBDD {

using LibCore::expr_group_t;
using LibCore::expr_groups_t;
using LibCore::symbol_t;
using LibCore::symbolic_reads_t;
using LibCore::SymbolManager;
using LibCore::Symbols;

using bdd_node_id_t  = u64;
using bdd_node_ids_t = std::unordered_set<bdd_node_id_t>;

class BDDVisitor;
class BDDNodeManager;
class Call;
class Route;

enum class BDDNodeType { Branch, Call, Route };
enum class BDDNodeVisitAction { Continue, SkipChildren, Stop };

struct cookie_t {
  virtual ~cookie_t() {}
  virtual std::unique_ptr<cookie_t> clone() const = 0;
};

class BDDNode {
protected:
  bdd_node_id_t id;
  BDDNodeType type;

  BDDNode *next;
  BDDNode *prev;

  SymbolManager *symbol_manager;

public:
  BDDNode(bdd_node_id_t _id, BDDNodeType _type, SymbolManager *_symbol_manager)
      : id(_id), type(_type), next(nullptr), prev(nullptr), symbol_manager(_symbol_manager) {}

  BDDNode(bdd_node_id_t _id, BDDNodeType _type, BDDNode *_next, BDDNode *_prev, SymbolManager *_symbol_manager)
      : id(_id), type(_type), next(_next), prev(_prev), symbol_manager(_symbol_manager) {}

  const BDDNode *get_next() const { return next; }
  const BDDNode *get_prev() const { return prev; }

  void set_next(BDDNode *_next) { next = _next; }
  void set_prev(BDDNode *_prev) { prev = _prev; }

  BDDNodeType get_type() const { return type; }
  bdd_node_id_t get_id() const { return id; }

  std::vector<klee::ref<klee::Expr>> get_ordered_branch_constraints() const;

  BDDNode *get_mutable_next() { return next; }
  BDDNode *get_mutable_prev() { return prev; }

  const BDDNode *get_node_by_id(bdd_node_id_t id) const;
  BDDNode *get_mutable_node_by_id(bdd_node_id_t id);

  void visit(BDDVisitor &visitor) const;
  void visit_nodes(std::function<BDDNodeVisitAction(const BDDNode *)> fn) const;
  void visit_mutable_nodes(std::function<BDDNodeVisitAction(BDDNode *)> fn);

  void visit_nodes(std::function<BDDNodeVisitAction(const BDDNode *, cookie_t *)> fn, std::unique_ptr<cookie_t> cookie) const;
  void visit_mutable_nodes(std::function<BDDNodeVisitAction(BDDNode *, cookie_t *)> fn, std::unique_ptr<cookie_t> cookie);

  void recursive_update_ids(bdd_node_id_t &new_id);
  void recursive_translate_symbol(const symbol_t &old_symbol, const symbol_t &new_symbol);
  void recursive_free_children(BDDNodeManager &manager);
  std::string recursive_dump(int lvl = 0) const;

  bool equals(const BDDNode *other) const;
  bool is_reachable(bdd_node_id_t id) const;
  bool is_packet_drop_code_path() const;
  std::string hash(bool recursive) const;
  size_t count_children(bool recursive) const;
  size_t count_code_paths() const;
  Symbols get_used_symbols() const;
  symbolic_reads_t get_used_symbolic_reads() const;
  std::vector<expr_groups_t> get_expr_groups() const;
  Symbols get_prev_symbols(const bdd_node_ids_t &stop_nodes = bdd_node_ids_t()) const;
  std::vector<const Call *> get_prev_functions(const std::unordered_set<std::string> &functions_names,
                                               const bdd_node_ids_t &stop_nodes = bdd_node_ids_t()) const;
  std::vector<const Call *> get_future_functions(const std::unordered_set<std::string> &functions, bool stop_on_branches = false) const;
  std::vector<const Call *> get_coalescing_nodes_from_key(klee::ref<klee::Expr> target_key, const map_coalescing_objs_t &data) const;
  std::vector<const Route *> get_future_routing_decisions() const;

  const Route *get_latest_routing_decision() const;
  Route *get_mutable_latest_routing_decision();

  std::vector<const BDDNode *> get_reachable_nodes() const;
  std::vector<BDDNode *> get_mutable_reachable_nodes();

  virtual std::vector<const BDDNode *> get_leaves() const;
  virtual std::vector<BDDNode *> get_mutable_leaves();
  virtual BDDNode *clone(BDDNodeManager &manager, bool recursive = false) const     = 0;
  virtual std::string dump(bool one_liner = false, bool id_name_only = false) const = 0;

  virtual ~BDDNode() = default;
};

} // namespace LibBDD
