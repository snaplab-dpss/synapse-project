#pragma once

#include <functional>
#include <vector>

#include "../types.h"

class Module;
class EPVisitor;
class EP;

enum class EPNodeVisitAction { Continue, SkipChildren, Stop };

class EPNode {
private:
  ep_node_id_t id;
  Module *module;
  std::vector<EPNode *> children;
  EPNode *prev;
  klee::ref<klee::Expr> constraint;

public:
  EPNode(Module *module);

  EPNode(const EPNode &other) = delete;
  EPNode(EPNode &&other) = delete;
  EPNode &operator=(const EPNode &other) = delete;

  ~EPNode();

  ep_node_id_t get_id() const;

  const Module *get_module() const;
  Module *get_mutable_module();

  const std::vector<EPNode *> &get_children() const;
  EPNode *get_prev() const;

  void set_id(ep_node_id_t id);
  void set_children(EPNode *child);
  void set_children(EPNode *on_true, EPNode *on_false);
  void set_prev(EPNode *prev);

  const EPNode *get_node_by_id(ep_node_id_t target_id) const;
  EPNode *get_mutable_node_by_id(ep_node_id_t target_id);

  void set_constraint(klee::ref<klee::Expr> constraint);
  klee::ref<klee::Expr> get_constraint() const;
  constraints_t get_constraints() const;

  std::string dump() const;
  EPNode *clone(bool recursive = false) const;

  void visit_nodes(std::function<EPNodeVisitAction(const EPNode *)> fn) const;
  void visit_mutable_nodes(std::function<EPNodeVisitAction(EPNode *)> fn);
};