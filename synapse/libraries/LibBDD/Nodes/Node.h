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

class BDDVisitor;
class NodeManager;
class Call;

using node_id_t  = u64;
using node_ids_t = std::unordered_set<node_id_t>;

enum class NodeType { Branch, Call, Route };
enum class NodeVisitAction { Continue, SkipChildren, Stop };

struct cookie_t {
  virtual ~cookie_t() {}
  virtual std::unique_ptr<cookie_t> clone() const = 0;
};

class Node {
protected:
  node_id_t id;
  NodeType type;

  Node *next;
  Node *prev;

  klee::ConstraintManager constraints;
  LibCore::SymbolManager *symbol_manager;

public:
  Node(node_id_t _id, NodeType _type, const klee::ConstraintManager &_constraints, LibCore::SymbolManager *_symbol_manager)
      : id(_id), type(_type), next(nullptr), prev(nullptr), constraints(_constraints), symbol_manager(_symbol_manager) {}

  Node(node_id_t _id, NodeType _type, Node *_next, Node *_prev, const klee::ConstraintManager &_constraints,
       LibCore::SymbolManager *_symbol_manager)
      : id(_id), type(_type), next(_next), prev(_prev), constraints(_constraints), symbol_manager(_symbol_manager) {}

  const Node *get_next() const { return next; }
  const Node *get_prev() const { return prev; }

  void set_next(Node *_next) { next = _next; }
  void set_prev(Node *_prev) { prev = _prev; }

  NodeType get_type() const { return type; }
  node_id_t get_id() const { return id; }

  const klee::ConstraintManager &get_constraints() const { return constraints; }
  void set_constraints(const klee::ConstraintManager &_constraints) { constraints = _constraints; }
  std::vector<klee::ref<klee::Expr>> get_ordered_branch_constraints() const;

  Node *get_mutable_next() { return next; }
  Node *get_mutable_prev() { return prev; }

  const Node *get_node_by_id(node_id_t id) const;
  Node *get_mutable_node_by_id(node_id_t id);

  void visit(BDDVisitor &visitor) const;
  void visit_nodes(std::function<NodeVisitAction(const Node *)> fn) const;
  void visit_mutable_nodes(std::function<NodeVisitAction(Node *)> fn);

  void visit_nodes(std::function<NodeVisitAction(const Node *, cookie_t *)> fn, std::unique_ptr<cookie_t> cookie) const;
  void visit_mutable_nodes(std::function<NodeVisitAction(Node *, cookie_t *)> fn, std::unique_ptr<cookie_t> cookie);

  void recursive_update_ids(node_id_t &new_id);
  void recursive_translate_symbol(LibCore::SymbolManager *symbol_manager, const LibCore::symbol_t &old_symbol,
                                  const LibCore::symbol_t &new_symbol);
  void recursive_add_constraint(klee::ref<klee::Expr> constraint);
  void recursive_free_children(NodeManager &manager);
  std::string recursive_dump(int lvl = 0) const;

  bool is_reachable(node_id_t id) const;
  bool is_packet_drop_code_path() const;
  std::string hash(bool recursive) const;
  size_t count_children(bool recursive) const;
  size_t count_code_paths() const;
  LibCore::Symbols get_used_symbols() const;
  LibCore::symbolic_reads_t get_used_symbolic_reads() const;
  LibCore::Symbols get_prev_symbols(const node_ids_t &stop_nodes = node_ids_t()) const;
  std::vector<const Call *> get_prev_functions(const std::unordered_set<std::string> &functions_names,
                                               const node_ids_t &stop_nodes = node_ids_t()) const;
  std::vector<const Call *> get_future_functions(const std::unordered_set<std::string> &functions, bool stop_on_branches = false) const;
  std::vector<const Call *> get_coalescing_nodes_from_key(klee::ref<klee::Expr> target_key, const map_coalescing_objs_t &data) const;

  virtual std::vector<node_id_t> get_leaves() const;
  virtual std::vector<const Node *> get_children(bool recursive = false) const;
  virtual Node *clone(NodeManager &manager, bool recursive = false) const           = 0;
  virtual std::string dump(bool one_liner = false, bool id_name_only = false) const = 0;

  virtual ~Node() = default;
};

} // namespace LibBDD